use std::{sync::Arc, time::Instant};

use anyhow::{Context, Result};
use libhydrogen::{kx, secretbox};
use netprotocol::{
    async_stream::AsyncStream,
    ghost::{Ghost, GhostRequest, GhostResponse},
};
use parking_lot::RwLock;

const TIME_TO_LIVE: u64 = 60 * 60;
type RegionalLeaderboard = [tokio::sync::RwLock<Option<Leaderboard>>; 32];

#[derive(Debug)]
struct Leaderboard {
    response: GhostResponse,
    time_created: Instant,
}

impl Default for Leaderboard {
    fn default() -> Self {
        Self {
            response: GhostResponse::default(),
            time_created: Instant::now(),
        }
    }
}

#[derive(Debug, Default)]
struct Cache {
    world: RegionalLeaderboard,
    americas: RegionalLeaderboard,
    europe: RegionalLeaderboard,
    asia: RegionalLeaderboard,
    oceania: RegionalLeaderboard,
}

impl Cache {
    fn leaderboards(&self, region: &str) -> Option<&RegionalLeaderboard> {
        Some(match region {
            "world" => &self.world,
            "americas" => &self.americas,
            "europe" => &self.europe,
            "asia" => &self.asia,
            "oceania" => &self.oceania,
            _ => return None,
        })
    }
}

async fn fetch_leaderboard(
    existing_leaderboard: &mut Option<Leaderboard>,
    session: &reqwest::Client,
    region: &str,
    track: usize,
) -> Result<()> {
    #[derive(serde::Deserialize, Debug)]
    struct MklResponse {
        score: u32,
    }

    #[derive(serde::Deserialize, Debug)]
    struct MklResponses {
        data: Vec<MklResponse>,
    }

    let mkl_responses: MklResponses = session
        .get(format!("https://mkleaderboards.com/api/charts/mkw_nonsc_{region}/{track}"))
        .send()
        .await?
        .error_for_status()?
        .json()
        .await?;

    let mut response = GhostResponse::default();
    response.ghosts.extend(mkl_responses.data.into_iter().map(|r| Ghost {
        minutes: ((r.score / 1000) - (r.score / 1000) % 60) / 60,
        seconds: r.score / 1000 % 60,
        millis: r.score % 1000,
    }));

    *existing_leaderboard = Some(Leaderboard {
        response,
        time_created: Instant::now(),
    });

    Ok(())
}

async fn handle_client(
    stream: tokio::net::TcpStream,
    server_keypair: kx::KeyPair,
    client: reqwest::Client,
    cache: Arc<Cache>,
) -> Result<()> {
    let context = secretbox::Context::from(*b"ghost   ");
    let mut connection =
        AsyncStream::<GhostRequest, GhostResponse>::new(stream, server_keypair, context).await?;

    loop {
        let Some(GhostRequest {region, track}) = connection.read().await? else {return Ok(())};
        let track = track as usize;

        let response = match cache.leaderboards(&region).and_then(|l| l.get(track)) {
            Some(leaderboard_lock) => {
                let leaderboard_read = leaderboard_lock.read().await;
                if let Some(leaderboard) = &*leaderboard_read {
                    let time_created = leaderboard.time_created;
                    if time_created.elapsed().as_secs() > TIME_TO_LIVE {
                        drop(leaderboard_read);
                        let mut leaderboard_write = leaderboard_lock.write().await;
                        if leaderboard_write.as_mut().unwrap().time_created == time_created {
                            fetch_leaderboard(&mut leaderboard_write, &client, &region, track)
                                .await?;
                        }

                        leaderboard_write.as_mut().unwrap().response.clone()
                    } else {
                        leaderboard.response.clone()
                    }
                } else {
                    drop(leaderboard_read);
                    let mut leaderboard_write = leaderboard_lock.write().await;
                    if leaderboard_write.is_none() {
                        fetch_leaderboard(&mut leaderboard_write, &client, &region, track).await?;
                    }

                    leaderboard_write.as_mut().unwrap().response.clone()
                }
            }
            None => continue,
        };

        connection.write(&response).await.context("Unable to respond")?;
    }
}

#[tokio::main]
async fn main() -> Result<()> {
    libhydrogen::init()?;
    tracing_subscriber::fmt::init();

    let session = reqwest::Client::new();
    let cache = Arc::new(Cache::default());

    let server_keypair = kx::KeyPair::gen();
    let listener = tokio::net::TcpListener::bind("0.0.0.0:21333").await?;

    loop {
        let (stream, peer_addr) = match listener.accept().await {
            Ok((stream, peer_addr)) => {
                tracing::info!("{peer_addr}: Accepted connection");
                (stream, peer_addr)
            }
            Err(e) => {
                tracing::error!("Failed to accept connection: {e}");
                continue;
            }
        };

        let cache = cache.clone();
        let session = session.clone();
        let server_keypair = server_keypair.clone();

        tokio::spawn(async move {
            match handle_client(stream, server_keypair, session, cache).await {
                Ok(()) => {
                    tracing::info!("{peer_addr}: Successfully initialized client");
                }
                Err(e) => {
                    tracing::error!("{peer_addr}: Failed to initialize client: {e}");
                }
            }
        });
    }
}
