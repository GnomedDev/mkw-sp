use std::{str::FromStr, time::Duration};

use axum::{
    extract::{Query, State},
    response::Redirect,
};
use time::{OffsetDateTime, PrimitiveDateTime};

use crate::{
    http::{
        error::{Error, Result},
        shared::{DISCORD_USER_AGENT, FRONTEND_ORIGIN},
        AppState,
    },
    wrappers::Sha1,
};

#[derive(serde::Deserialize)]
struct DiscordUser {
    id: String,
    avatar: String,
    username: String,
}

async fn fetch_discord_info(reqwest: reqwest::Client, access_token: &str) -> Result<DiscordUser> {
    #[derive(serde::Deserialize)]
    struct AuthorizationResponse {
        user: DiscordUser,
    }

    let response: AuthorizationResponse = reqwest
        .get("https://discord.com/api/v10/oauth2/@me")
        .bearer_auth(access_token)
        .header("User-Agent", DISCORD_USER_AGENT)
        .send()
        .await
        .map_err(|e| Error::Unknown(e.into()))?
        .error_for_status()
        .map_err(|_| Error::InvalidAuthorization)?
        .json()
        .await
        .map_err(|e| Error::Unknown(e.into()))?;

    Ok(response.user)
}

async fn insert_user(
    pool: sqlx::PgPool,
    reqwest: reqwest::Client,
    access_token: &str,
    expires_in: u32,
) -> Result<()> {
    let user = fetch_discord_info(reqwest, access_token).await?;
    let user_id = i64::from_str(&user.id).map_err(|e| Error::Unknown(e.into()))?;

    let expiry = {
        let expiry = OffsetDateTime::now_utc() + Duration::from_secs(expires_in as u64);
        PrimitiveDateTime::new(expiry.date(), expiry.time())
    };

    let query = sqlx::query!(
        "INSERT INTO users (discord_id, name, pfp_hash, expiry, token_hash)
            VALUES($1, $2, $3, $4, $5)
        ON CONFLICT(discord_id) DO UPDATE
            SET name = $2, pfp_hash = $3, expiry = $4, token_hash = $5",
        user_id,
        user.username,
        user.avatar,
        expiry,
        &Sha1::hash(access_token.as_bytes()).into_inner()
    );

    query.execute(&pool).await?;
    Ok(())
}

#[derive(serde::Deserialize)]
pub struct OAuthRedirect {
    code: String,
}

pub async fn oauth_endpoint(
    State(state): State<AppState>,
    Query(query): Query<OAuthRedirect>,
) -> Result<Redirect> {
    #[derive(serde::Serialize)]
    struct TokenExchange<'a> {
        client_id: &'a str,
        client_secret: &'a str,
        grant_type: &'static str,
        code: &'a str,
        redirect_uri: &'static str,
    }

    #[derive(serde::Deserialize)]
    struct TokenResponse {
        access_token: String,
        token_type: String,
        expires_in: u32,
    }

    let exchange = TokenExchange {
        client_id: &state.oauth.client_id,
        client_secret: &state.oauth.client_secret,
        grant_type: "authorization_code",
        code: &query.code,
        #[cfg(debug_assertions)]
        redirect_uri: "http://localhost:3000/oauth",
        #[cfg(not(debug_assertions))]
        redirect_uri: "https://backend.mkw-spc.com/oauth",
    };

    let TokenResponse {
        access_token,
        token_type,
        expires_in,
    } = state
        .reqwest
        .post("https://discord.com/api/oauth2/token")
        .header("User-Agent", DISCORD_USER_AGENT)
        .form(&exchange)
        .send()
        .await
        .map_err(|e| Error::Unknown(e.into()))?
        .error_for_status()
        .map_err(|e| Error::Unknown(e.into()))?
        .json()
        .await
        .map_err(|e| Error::Unknown(e.into()))?;

    if token_type != "Bearer" {
        return Err(Error::Unknown(anyhow::anyhow!(
            "Discord returned token type other than Bearer!"
        )));
    }

    insert_user(state.pack_db, state.reqwest, &access_token, expires_in).await?;

    let redirect_back = format!("{FRONTEND_ORIGIN}/pack-upload?access_token={access_token}");
    Ok(Redirect::permanent(&redirect_back))
}
