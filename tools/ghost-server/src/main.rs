use std::sync::Arc;

use anyhow::{Context, Result};
use libhydrogen::{kx, secretbox};
use netprotocol::{
    async_stream::AsyncStream,
    ghost::{Ghost, GhostRequest, GhostResponse},
};
use parking_lot::RwLock;

mod leaderboard;

#[derive(Debug, Default)]
struct Cache {
    world: [GhostResponse; 32],
    americas: [GhostResponse; 32],
    europe: [GhostResponse; 32],
    asia: [GhostResponse; 32],
    oceania: [GhostResponse; 32],
}

impl Cache {
    fn leaderboards(&self, region: &str) -> Option<&[GhostResponse; 32]> {
        Some(match region {
            "world" => &self.world,
            "americas" => &self.americas,
            "europe" => &self.europe,
            "asia" => &self.asia,
            "oceania" => &self.oceania,
            _ => return None,
        })
    }
    fn leaderboards_mut(&mut self, region: &str) -> Option<&mut [GhostResponse; 32]> {
        Some(match region {
            "world" => &mut self.world,
            "americas" => &mut self.americas,
            "europe" => &mut self.europe,
            "asia" => &mut self.asia,
            "oceania" => &mut self.oceania,
            _ => return None,
        })
    }
}

async fn fetch_leaderboard(
    cache: Arc<RwLock<Cache>>,
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

    cache.write().leaderboards_mut(region).unwrap()[track - 49] = response;
    Ok(())
}

async fn fetch_leaderboards(session: &reqwest::Client) -> Result<Arc<RwLock<Cache>>> {
    let mut tasks = Vec::new();
    let cache = Arc::new(RwLock::new(Cache::default()));

    for region in ["world", "americas", "europe", "asia", "oceania"] {
        for track in 49..81 {
            let fut = fetch_leaderboard(cache.clone(), session, region, track);
            tasks.push(fut)
        }
    }

    println!("{}", tasks.len());
    futures::future::try_join_all(tasks).await?;
    Ok(dbg!(cache))
}

async fn handle_client(
    stream: tokio::net::TcpStream,
    server_keypair: kx::KeyPair,
    cache: Arc<RwLock<Cache>>,
) -> Result<()> {
    let context = secretbox::Context::from(*b"ghost   ");
    let mut connection =
        AsyncStream::<GhostRequest, _>::new(stream, server_keypair, context).await?;

    loop {
        let Some(request) = connection.read().await? else {return Ok(())};
        let response = match cache
            .read()
            .leaderboards(&request.region)
            .and_then(|l| l.get(request.track as usize))
        {
            Some(leaderboard) => leaderboard.clone(),
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
    let cache = fetch_leaderboards(&session).await?;

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
        let server_keypair = server_keypair.clone();

        tokio::spawn(async move {
            match handle_client(stream, server_keypair, cache).await {
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
