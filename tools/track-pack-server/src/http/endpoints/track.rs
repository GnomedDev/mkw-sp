use axum::{
    extract::{Query, State},
    Json,
};
use netprotocol::pack_serving::ProtoSha1;
use sqlx::Either;
use tokio_stream::StreamExt;

use crate::{
    http::{
        error::Result,
        shared::{fetch_user, TrackResponse},
        AppState,
    },
    track_database::{extract_version, is_battle},
    wrappers::Sha1,
};

async fn fetch_unreleased_track(
    conn: &mut sqlx::PgConnection,
    discord_id: i64,
    track: Sha1,
) -> Result<Option<TrackResponse>, sqlx::Error> {
    let query = sqlx::query!(
        "SELECT track_hash, name, version, course_id FROM unreleased_tracks
        WHERE track_hash = $1 AND approved = false AND author_id = $2",
        &track.into_inner(),
        discord_id
    );

    let mut stream = query.fetch_many(conn);
    let mut response: Option<TrackResponse> = None;
    while let Some(track) = stream.next().await {
        let Either::Right(track) = track? else {
            continue;
        };

        if let Some(response) = &mut response {
            let hash = ProtoSha1 {
                data: track.track_hash,
            };

            response.versions.push((track.version, hash.try_into().unwrap()));
        } else {
            response = Some(TrackResponse {
                name: track.name,
                versions: Vec::new(),
                is_battle: is_battle(track.course_id),
            });
        }
    }

    Ok(response)
}

#[derive(serde::Deserialize)]
pub struct TrackParams {
    query: Sha1,
    access_token: String,
}

pub async fn track_endpoint(
    State(state): State<AppState>,
    Query(TrackParams {
        access_token,
        query: track,
    }): Query<TrackParams>,
) -> Result<Json<Option<TrackResponse>>> {
    let mut conn = state.pack_db.acquire().await?;
    let discord_id = fetch_user(&mut conn, &access_token).await?;

    let mut response = if let Some(track) = state.track_db.get(&track) {
        let versions: Vec<(String, Sha1)> = state
            .track_db
            .iter()
            .filter(|i| track.name == i.value().name && track.course_id == i.value().course_id)
            .map(|i| (i.value().version.clone().unwrap(), *i.key()))
            .collect();

        #[allow(clippy::cast_possible_truncation)]
        TrackResponse {
            is_battle: is_battle(track.course_id as i16),
            name: track.name.clone(),
            versions,
        }
    } else if let Some(response) = fetch_unreleased_track(&mut conn, discord_id, track).await? {
        response
    } else {
        return Ok(Json(None));
    };

    response.versions.sort_by_key(|(v, _)| extract_version(v));
    Ok(Json(Some(response)))
}
