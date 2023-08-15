use axum::{
    extract::{Query, State},
    Json,
};
use edit_distance::edit_distance;

use crate::{
    http::{error::Result, shared::TrackResponse, AppState},
    track_database::{extract_version, is_battle},
};

#[derive(serde::Deserialize)]
pub struct SearchParams {
    query: String,
}

#[derive(serde::Serialize, Default)]
#[serde(transparent)]
pub struct SearchResponse {
    pub tracks: Vec<TrackResponse>,
}

pub async fn search_endpoint(
    State(state): State<AppState>,
    Query(params): Query<SearchParams>,
) -> Result<Json<SearchResponse>> {
    if params.query.len() <= 2 {
        return Ok(Json(SearchResponse::default()));
    }

    #[allow(clippy::cast_possible_truncation)]
    let mut tracks: Vec<_> = state
        .track_db
        .iter()
        .map(|item| {
            // Extract hash, name, and version
            let (hash, track) = item.pair();
            (
                *hash,
                is_battle(track.course_id as i16),
                track.name.clone(),
                track.version.clone().unwrap(),
            )
        })
        .filter(|(_, _, name, _)| {
            let name = name.to_lowercase();
            let query = params.query.to_lowercase();
            name.starts_with(&query)
        }) // Filter out names that do not start with query
        .collect();

    // Sort by edit distance
    tracks.sort_by_key(|(_, _, name, _)| edit_distance(name, &params.query));
    // Now that it's sorted, deduplicate any left over entries
    tracks.dedup_by(|(_, _, name1, ver1), (_, _, name2, ver2)| name1 == name2 && ver1 == ver2);

    let mut response = SearchResponse::default();
    for (hash, is_battle, name, version) in tracks {
        if let Some(track) = response.tracks.iter_mut().find(|t| t.name == name) {
            track.versions.push((version, hash));
        } else {
            response.tracks.push(TrackResponse {
                name,
                is_battle,
                versions: Vec::new(),
            });

            response.tracks.last_mut().unwrap().versions.push((version.clone(), hash));
        }

        if response.tracks.len() == 5 {
            break;
        }
    }

    for track in &mut response.tracks {
        track.versions.sort_by_key(|(v, _)| extract_version(v));
    }

    Ok(Json::from(response))
}
