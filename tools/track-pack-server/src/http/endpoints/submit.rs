use axum::{extract::State, Json};
use prost::Message;
use sqlx::Connection;

use netprotocol::pack_serving::{Pack, ProtoSha1};

use crate::{
    http::{
        error::{Error, Result},
        shared::fetch_user,
        AppState,
    },
    pack_list::TrackType,
    wrappers::Sha1,
};

fn filter_tracks(tracks: &[(TrackType, Sha1)], track_type: TrackType) -> Vec<ProtoSha1> {
    tracks.iter().filter(|(kind, _)| *kind == track_type).map(|(_, hash)| hash.into()).collect()
}

async fn get_pending_pack_count(conn: &mut sqlx::PgConnection, author_id: i64) -> Result<i64> {
    let query = sqlx::query!(
        "SELECT COUNT(*) FROM packs WHERE author_id = $1 AND approved = false",
        author_id
    );

    let response = query.fetch_one(conn).await?;
    Ok(response.count.unwrap_or(0))
}

async fn insert_track(
    transaction: &mut sqlx::Transaction<'_, sqlx::Postgres>,
    pack_hash: Sha1,
    track_hash: Sha1,
    track_type: TrackType,
) -> Result<(), sqlx::Error> {
    // We have to use the non-macro version of sqlx query due to the custom TrackType.
    let query =
        sqlx::query("INSERT INTO pack_contents(pack_hash, track_hash, type) VALUES($1, $2, $3)");
    let query = query.bind(pack_hash.into_inner()).bind(track_hash.into_inner()).bind(track_type);
    query.execute(&mut **transaction).await.map(drop)
}

async fn insert_into_db(
    body: SubmitParams,
    pack_hash: Sha1,
    author_id: i64,
    transaction: &mut sqlx::Transaction<'_, sqlx::Postgres>,
) -> Result<(), sqlx::Error> {
    let query = sqlx::query!(
        "INSERT INTO packs (hash, author_id, name, description)
        VALUES             ($1,   $2,        $3,   $4)",
        &pack_hash.into_inner(),
        author_id,
        body.name,
        body.description
    );

    query.execute(&mut **transaction).await?;

    for (track_type, track_hash) in body.tracks {
        insert_track(transaction, pack_hash, track_hash, track_type).await?;
    }

    Ok(())
}

#[derive(serde::Deserialize)]
pub struct SubmitParams {
    name: String,
    description: String,
    author_names: String,

    tracks: Vec<(TrackType, Sha1)>,
    access_token: String,
}

pub async fn submit_endpoint(
    State(state): State<AppState>,
    Json(body): Json<SubmitParams>,
) -> Result<String> {
    if body.name.len() >= 64 {
        return Err(Error::InvalidSubmitReq("Name must be under 64 characters long."));
    }

    if body.description.len() >= 128 {
        return Err(Error::InvalidSubmitReq("Description must be under 128 characters long."));
    }

    if body.author_names.len() >= 64 {
        return Err(Error::InvalidSubmitReq("Author names must be under 64 characters long."));
    }

    // Create/update the user info for the pack author
    let mut db_conn = state.pack_db.acquire().await?;
    let author_id = fetch_user(&mut db_conn, &body.access_token).await?;

    // Check if the user already has a pack pending, if so, tell them to wait
    if get_pending_pack_count(&mut db_conn, author_id).await? != 0 {
        return Err(Error::AlreadyPending);
    }

    // Calculate the hash of the pack, by just protobuf encoding it and hashing.
    let pack_hash = {
        let proto_pack = Pack {
            name: body.name.clone(),
            author_names: body.author_names.clone(),
            description: Some(body.description.clone()),
            race_tracks: filter_tracks(&body.tracks, TrackType::Race),
            coin_tracks: filter_tracks(&body.tracks, TrackType::Coin),
            balloon_tracks: filter_tracks(&body.tracks, TrackType::Balloon),
        };

        let pack_encoded = proto_pack.encode_to_vec();
        Sha1::hash(&pack_encoded)
    };

    // Insert all tracks and the pack into the database, all at once in a transaction.
    // This avoids half-inserted/dangling tracks and other inconsistencies.
    let res = db_conn
        .transaction(move |trans| Box::pin(insert_into_db(body, pack_hash, author_id, trans)));

    if let Err(err) = res.await {
        if let sqlx::Error::Database(err) = &err {
            if err.kind() == sqlx::error::ErrorKind::UniqueViolation {
                return Err(Error::InvalidSubmitReq("Pack has already been submitted!"));
            }
        }

        Err(Error::Unknown(err.into()))
    } else {
        Ok(hex::encode(pack_hash.into_inner()))
    }
}
