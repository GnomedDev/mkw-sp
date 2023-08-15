use axum::extract::multipart::MultipartError;
use reqwest::StatusCode;

#[derive(Debug, thiserror::Error)]
pub struct PanicWrapper(pub Box<dyn std::any::Any + Send>);

impl std::fmt::Display for PanicWrapper {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("Panicked")
    }
}

#[derive(Debug, thiserror::Error)]
#[rustfmt::skip]
pub enum TrackFileError {
    #[error("Failed to decode wbz/wu8 track file")]
    Decoding(#[from] #[source] wbz_converter::Error),
    #[error("Panicked while decoding wbz/wu8 track file")]
    Panicked(#[source] PanicWrapper),
    #[error("wbz/wu8 file is too short to be valid")]
    TooShort,
}

#[derive(Debug, thiserror::Error)]
#[rustfmt::skip]
pub enum Error {
    #[error("{0}")]
    BadJson(#[from] #[source] serde_json::Error),
    #[error("Failed to communicate with database")]
    Database(#[from] #[source] sqlx::Error),
    #[error("Failed to authenticate with discord")]
    InvalidAuthorization,
    #[error("Unexpected error")]
    Unknown(#[from] #[source] anyhow::Error),

    #[error("{0}")]
    InvalidTrackFile(#[from] #[source] TrackFileError),
    #[error("{0}")]
    InvalidSubmitReq(&'static str),
    #[error("You already have a pack pending approval!")]
    AlreadyPending,

    #[error("Missing field from miltipart post")]
    MissingField,
    #[error("{0}")]
    DecodingField(#[from] #[source] MultipartError),
}

impl Error {
    fn status_code(&self) -> StatusCode {
        match self {
            Self::BadJson(_)
            | Self::MissingField
            | Self::InvalidTrackFile(_)
            | Self::InvalidSubmitReq(_) => StatusCode::BAD_REQUEST,
            Self::Unknown(_) | Self::Database(_) => StatusCode::INTERNAL_SERVER_ERROR,
            Self::InvalidAuthorization => StatusCode::UNAUTHORIZED,
            Self::AlreadyPending => StatusCode::TOO_MANY_REQUESTS,
            Self::DecodingField(error) => error.status(),
        }
    }
}

impl axum::response::IntoResponse for Error {
    fn into_response(self) -> axum::response::Response {
        if let Self::Unknown(err) = &self {
            tracing::error!("Failed to handle HTTP request: {err}");
        } else if let Self::Database(err) = &self {
            tracing::error!("Failed to communicate with database: {err}");
        } else if let Self::InvalidTrackFile(err) = &self {
            tracing::warn!("{err}");
        }

        axum::response::Response::builder()
            .status(self.status_code())
            .body(axum::body::boxed(self.to_string()))
            .unwrap()
    }
}

pub(super) type Result<T, E = Error> = std::result::Result<T, E>;
