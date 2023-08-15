use std::path::Path;

use axum::{
    extract::{Multipart, State},
    Json,
};

use crate::{
    http::{
        error::{Error, PanicWrapper, Result, TrackFileError},
        shared::fetch_user,
        AppState,
    },
    wrappers::Sha1,
};

fn get_track_hash(track: &[u8], autoadd_path: &Path) -> Result<Sha1, TrackFileError> {
    if track.len() < 0x20 {
        return Err(TrackFileError::TooShort);
    }

    let magic_bytes: [u8; 8] = track[0..8].try_into().unwrap();
    let track_data = if magic_bytes == *b"WBZaWU8a" {
        let wbz_cursor = std::io::Cursor::new(track);
        wbz_converter::decode_wbz(wbz_cursor, autoadd_path)?
    } else if &magic_bytes[0..4] == b"WU8a" {
        let mut buffer = track.to_vec();
        wbz_converter::decode_wu8(&mut buffer, autoadd_path)?;
        buffer
    } else {
        return Err(TrackFileError::Decoding(wbz_converter::Error::InvalidWBZMagic {
            found_magic: magic_bytes,
        }));
    };

    Ok(Sha1::hash(&track_data))
}

async fn insert_unreleased_track(
    transaction: impl sqlx::PgExecutor<'_>,
    track_hash: Sha1,
    author_id: i64,
    track: TrackData,
    track_file: &[u8],
) -> Result<(), sqlx::Error> {
    let query = sqlx::query!(
        "INSERT INTO
            unreleased_tracks(track_hash, author_id, track_contents, name, version, course_id, music_id)
            VALUES           ($1,         $2,        $3,             $4,   $5,      $6,        $7)",
        &track_hash.into_inner(),
        author_id,
        track_file,
        &track.name,
        &track.version,
        track.course as i16,
        track.music as i16,
    );

    query.execute(transaction).await.map(drop)
}

#[derive(serde::Deserialize)]
struct TrackData {
    name: String,
    version: String,
    course: u8,
    music: u8,
    access_token: String,
}

#[derive(serde::Serialize)]
pub struct TrackResponse {
    id: Sha1,
    is_new: bool,
}

pub async fn submit_track_endpoint(
    State(state): State<AppState>,
    mut data: Multipart,
) -> Result<Json<TrackResponse>> {
    let Some(metadata_raw) = data.next_field().await? else {
        return Err(Error::MissingField);
    };

    let metadata: TrackData = {
        let metadata_raw = metadata_raw.text().await?;
        serde_json::from_str(&metadata_raw)?
    };

    let mut db_conn = state.pack_db.acquire().await?;
    let user_id = fetch_user(&mut db_conn, &metadata.access_token).await?;

    let Some(track_data_raw) = data.next_field().await? else {
        return Err(Error::MissingField);
    };

    let track_data = track_data_raw.bytes().await?;
    let (track_data, track_hash) = {
        // Get the track's hash. Since this can require bzip decompression, run this in a thread.
        // This spawn_blocking also allows us to guard against any panics in wbz_converter.
        let task = move || (get_track_hash(&track_data, &state.autoadd_path), track_data);
        let (res, track_data) = tokio::task::spawn_blocking(task)
            .await
            .map_err(tokio::task::JoinError::into_panic)
            .map_err(PanicWrapper)
            .map_err(TrackFileError::Panicked)
            .map_err(Error::InvalidTrackFile)?;

        (track_data, res?)
    };

    let result = insert_unreleased_track(&mut *db_conn, track_hash, user_id, metadata, &track_data);

    let mut is_new = true;
    if let Err(sqlx::Error::Database(err)) = result.await {
        if err.is_unique_violation() {
            is_new = false;
        } else {
            return Err(Error::Database(sqlx::Error::Database(err)));
        }
    };

    Ok(Json(TrackResponse {
        is_new,
        id: track_hash,
    }))
}
