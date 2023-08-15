//! The HTTP API for Track Pack Server
//!
//! This API is used by the site to allow track authors to upload new tracks,
//! create new packs, and allow trusted users to approve new packs for hosting.

use std::{path::PathBuf, sync::Arc};

use anyhow::Context;
use axum::{
    http::HeaderValue,
    routing::{get, post},
    Router,
};
use reqwest::Method;

use crate::{http::shared::FRONTEND_ORIGIN, Fallible, Success, TrackDB};

mod endpoints;
mod error;
mod shared;

#[derive(Clone)]
pub struct OAuthState {
    client_id: String,
    client_secret: String,
}

#[derive(Clone)]
pub struct AppState {
    track_db: Arc<TrackDB>,
    pack_db: sqlx::PgPool,
    reqwest: reqwest::Client,
    autoadd_path: PathBuf,
    oauth: OAuthState,
}

const MEGABYTE: usize = 1000 * 1000;

pub async fn run(
    track_db: Arc<TrackDB>,
    pack_db: sqlx::PgPool,
    reqwest: reqwest::Client,
) -> Fallible {
    let oauth = OAuthState {
        client_id: std::env::var("OAUTH_CLIENT_ID").context("OAUTH_CLIENT_ID must be set")?,
        client_secret: std::env::var("OAUTH_CLIENT_SECRET")
            .context("OAUTH_CLIENT_SECRET must be set")?,
    };

    let state = AppState {
        autoadd_path: std::env::var("AUTOADD_PATH").unwrap_or(String::from("autoadd")).into(),
        track_db,
        pack_db,
        reqwest,
        oauth,
    };

    anyhow::ensure!(
        state.autoadd_path.exists(),
        "WBZ autoadd folder does not exist! Set the path with AUTOADD_PATH or place required files in `autoadd`"
    );

    let body_size_limit = axum::extract::DefaultBodyLimit::max(MEGABYTE * 15);

    let origin: HeaderValue = FRONTEND_ORIGIN.parse()?;
    let cors_layer = tower_http::cors::CorsLayer::new()
        .allow_methods([Method::GET, Method::POST, Method::OPTIONS])
        .allow_headers(tower_http::cors::Any)
        .allow_origin(origin);

    let app = Router::new()
        .route("/oauth", get(endpoints::oauth))
        .route("/search", get(endpoints::search))
        .route("/submit", post(endpoints::submit))
        .route("/track", get(endpoints::track))
        .route("/track", post(endpoints::submit_track))
        .layer(body_size_limit)
        .layer(cors_layer)
        .with_state(state);

    let bind_address = "0.0.0.0:3000".parse()?;
    tracing::info!("Binding to {bind_address}, ready for HTTP connections!");
    axum::Server::bind(&bind_address).serve(app.into_make_service()).await?;

    Success
}
