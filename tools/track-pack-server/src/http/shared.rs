use time::OffsetDateTime;

use crate::http::error::{Error, Result};
use crate::wrappers::Sha1;

pub const DISCORD_USER_AGENT: &str = "MKW-SPC (https://github.com/GnomedDev/mkw-spc, Rolling)";

#[cfg(debug_assertions)]
pub const FRONTEND_ORIGIN: &str = "http://localhost:4000";
#[cfg(not(debug_assertions))]
pub const FRONTEND_ORIGIN: &str = "https://mkw-spc.com";

#[derive(serde::Serialize)]
pub struct TrackResponse {
    pub name: String,
    pub is_battle: bool,
    pub versions: Vec<(String, Sha1)>,
}

pub async fn fetch_user(conn: &mut sqlx::PgConnection, access_token: &str) -> Result<i64> {
    let query = sqlx::query!(
        "SELECT discord_id, expiry FROM users WHERE token_hash = $1",
        &Sha1::hash(access_token.as_bytes()).into_inner()
    );

    let record = query
        .fetch_optional(conn)
        .await
        .transpose()
        .ok_or(Error::InvalidAuthorization)?
        .map_err(Error::Database)?;

    if record.expiry.assume_utc() > OffsetDateTime::now_utc() {
        Ok(record.discord_id)
    } else {
        Err(Error::InvalidAuthorization)
    }
}
