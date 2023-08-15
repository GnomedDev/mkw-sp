#![warn(clippy::pedantic)]
#![allow(
    clippy::cast_lossless,
    clippy::cast_sign_loss,
    clippy::cast_possible_wrap,
    clippy::module_name_repetitions
)]

use std::sync::Arc;

use anyhow::bail;
use anyhow::Result;
use arrayvec::ArrayVec;
use libhydrogen::secretbox;
use netprotocol::{
    pack_serving::{
        ctts_message, tstc_message, CTTSMessage, CTTSMessageOpt, Pack, ProtoSha1, TSTCMessage,
        TSTCMessageOpt,
    },
    ClientId, ClientIdOpt,
};
use pack_list::{connect_pack_db, DatabasePack};
use prost::Message;
use tokio_stream::StreamExt;
use track_database::{load_track_file, TrackDB};

mod http;
mod pack_list;
mod slot_lookup;
mod track_database;
mod wrappers;

type Fallible = Result<()>;
type AsyncStream = netprotocol::async_stream::AsyncStream<
    CTTSMessageOpt,
    TSTCMessageOpt,
    netprotocol::NNegotiator,
>;

#[allow(non_upper_case_globals)]
const Success: Fallible = Ok(());

async fn get_top_packs(conn: &mut sqlx::PgConnection) -> Result<ArrayVec<ProtoSha1, 5>> {
    let query = sqlx::query_as!(
        pack_list::DatabasePack,
        "SELECT * FROM packs WHERE approved = true ORDER BY downloads DESC LIMIT 5;"
    );

    let mut top_packs = ArrayVec::new();
    let mut db_iter = query.fetch_many(&mut *conn);
    while let Some(pack) = db_iter.next().await {
        let hash = ProtoSha1 {
            data: pack?.right().unwrap().hash,
        };

        if top_packs.try_push(hash).is_err() {
            break;
        }
    }

    Ok(top_packs)
}

async fn get_discord_id(
    conn: &mut sqlx::PgConnection,
    client_id_raw: ClientIdOpt,
) -> Result<Option<i64>> {
    if let ClientId::LoggedIn {
        device,
        licence,
    } = ClientId::from(client_id_raw)
    {
        let query = sqlx::query!(
            "SELECT discord_id FROM users WHERE device_id = $1 AND licence_id = $2",
            device as i32,
            licence as i16
        );

        Ok(query.fetch_optional(&mut *conn).await?.map(|u| u.discord_id))
    } else {
        Ok(None)
    }
}

async fn handle_connection(
    mut stream: AsyncStream,
    reqwest: &reqwest::Client,
    track_db: &TrackDB,
    pack_db: sqlx::PgPool,
) -> Fallible {
    let Some(CTTSMessageOpt {
        message: Some(CTTSMessage::Login(login_req)),
    }) = stream.read().await?
    else {
        bail!("Incorrect login message or disconnected before login!");
    };

    tracing::info!("Client connected with {} packs downloaded!", login_req.downloaded_packs.len());
    let (discord_id, top_packs) = {
        let mut conn = pack_db.acquire().await?;
        let top_packs = get_top_packs(&mut conn).await?;
        let discord_id = get_discord_id(&mut conn, login_req.id).await?;

        (discord_id, top_packs)
    };

    let login_response = tstc_message::LoginResponse {
        pack_updates: Vec::new(),
        top_packs: top_packs.to_vec(),
    };

    let message = TSTCMessageOpt {
        message: Some(TSTCMessage::LoginResponse(login_response)),
    };

    tracing::debug!("Sending login response: {:?}", message);
    stream.write(&message).await?;

    while let Some(msg) = stream.read().await? {
        let Some(msg) = msg.message else {
            break;
        };

        match msg {
            CTTSMessage::Login(_) => {
                tracing::warn!("Client sent duplicate login message?");
                break;
            }
            CTTSMessage::DownloadPack(ctts_message::DownloadPack {
                pack_id,
            }) => {
                let pack = {
                    let mut conn = pack_db.acquire().await?;
                    let query = sqlx::query_as!(
                        DatabasePack,
                        "SELECT * FROM packs WHERE hash = $1 and approved = true",
                        &pack_id.data,
                    );

                    let Some(db_pack) = query.fetch_optional(&mut *conn).await? else {
                        tracing::warn!("Client tried requesting non-existent pack");
                        break;
                    };

                    pack_list::generate_pack_manifest(&mut conn, db_pack).await?
                };

                let encoded_pack = pack.encode_to_vec();
                let message = TSTCMessageOpt {
                    message: Some(TSTCMessage::PackResponse(tstc_message::PackResponse {
                        pack_size: encoded_pack.len().try_into()?,
                    })),
                };

                stream.write(&message).await?;
                stream.write_raw(&encoded_pack).await?;
            }
            CTTSMessage::DownloadTrack(ctts_message::DownloadTrack {
                track_id,
            }) => {
                let track_file =
                    load_track_file(reqwest, track_db, &pack_db, discord_id.unwrap_or(0), track_id)
                        .await?;

                let Some((manifest, track_file)) = track_file else {
                    tracing::warn!("Client tried to request non-existant track");
                    return Ok(());
                };

                let message = TSTCMessageOpt {
                    message: Some(TSTCMessage::TrackResponse(tstc_message::TrackResponse {
                        manifest,
                        track_size: track_file
                            .len()
                            .try_into()
                            .map_err(|_| anyhow::anyhow!("Track file too large!"))?,
                    })),
                };

                stream.write(&message).await?;
                stream.write_raw(&track_file).await?;
            }
        }
    }

    Success
}

async fn handle_connections(
    listener: tokio::net::TcpListener,
    reqwest: reqwest::Client,
    pack_db: sqlx::PgPool,
    track_db: Arc<TrackDB>,
    server_keypair: libhydrogen::kx::KeyPair,
) -> Fallible {
    tracing::info!("Bound to {}, ready for TCP connections!", listener.local_addr()?);

    loop {
        let Ok((stream, addr)) = listener.accept().await else {
            continue;
        };
        tracing::debug!("Accepted connection from {addr}");

        let reqwest = reqwest.clone();
        let pack_db = pack_db.clone();
        let track_db = track_db.clone();
        let server_keypair = server_keypair.clone();

        tokio::spawn(async move {
            let context = secretbox::Context::from(*b"trackpak");
            let Ok(stream) = AsyncStream::new(stream, server_keypair, context).await else {
                tracing::error!("Failed to create tcp stream");
                return;
            };

            let res = handle_connection(stream, &reqwest, &track_db, pack_db).await;
            if let Err(err) = res {
                tracing::error!("Error in connection handler: {err}");
            } else {
                tracing::info!("Client disconnected!");
            };
        });
    }
}

#[tokio::main]
async fn main() -> Fallible {
    match dotenvy::dotenv() {
        Ok(_) | Err(dotenvy::Error::Io(_)) => {}
        Err(err) => return Err(err.into()),
    }

    tracing_subscriber::fmt::init();
    libhydrogen::init()?;

    let server_keypair = netprotocol::NNegotiator::generate_keypair()?;
    tracing::info!("Public Key: {:02x?}", server_keypair.public_key.as_ref());

    let reqwest = reqwest::Client::new();
    let (track_db, pack_db) = tokio::try_join!(track_database::parse(&reqwest), connect_pack_db())?;

    let track_db = Arc::new(track_db);

    let listener = tokio::net::TcpListener::bind("127.0.0.1:21333").await?;
    tokio::try_join!(
        http::run(Arc::clone(&track_db), pack_db.clone(), reqwest.clone()),
        handle_connections(listener, reqwest, pack_db, track_db, server_keypair),
    )?;

    Success
}
