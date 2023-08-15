use netprotocol::pack_serving::ProtoSha1;

use crate::{Pack, Result};

#[derive(serde::Deserialize, sqlx::Type, Clone, Copy, PartialEq, Eq)]
#[sqlx(rename_all = "lowercase")]
pub enum TrackType {
    Race,
    Coin,
    Balloon,
}

pub struct DatabasePack {
    pub hash: Vec<u8>,
    pub author_id: i64,
    pub approved: bool,
    pub downloads: i64,

    pub name: String,
    pub description: String,
}

#[derive(sqlx::FromRow)]
pub struct DatabaseTrack {
    pub pack_hash: Vec<u8>,
    pub track_hash: Vec<u8>,
    pub r#type: TrackType,
}

pub async fn generate_pack_manifest(
    conn: &mut sqlx::PgConnection,
    db_pack: DatabasePack,
) -> Result<Pack> {
    let author_name = {
        let query = sqlx::query!("SELECT name FROM users WHERE discord_id = $1", db_pack.author_id);
        query.fetch_one(&mut *conn).await?.name
    };

    let mut race_tracks = Vec::new();
    let mut coin_tracks = Vec::new();
    let mut balloon_tracks = Vec::new();
    let tracks = {
        let query = sqlx::query_as::<sqlx::Postgres, DatabaseTrack>(
            "SELECT * FROM pack_contents WHERE pack_hash = $1",
        );

        query.bind(&db_pack.hash).fetch_all(conn).await?
    };

    for db_track in tracks {
        anyhow::ensure!(db_track.track_hash.len() == 0x14);
        let hash = ProtoSha1 {
            data: db_track.track_hash,
        };

        match db_track.r#type {
            TrackType::Race => race_tracks.push(hash),
            TrackType::Coin => coin_tracks.push(hash),
            TrackType::Balloon => balloon_tracks.push(hash),
        }
    }

    Ok(Pack {
        race_tracks,
        coin_tracks,
        balloon_tracks,
        name: db_pack.name,
        author_names: author_name,
        description: Some(db_pack.description),
    })
}

pub async fn connect_pack_db() -> Result<sqlx::PgPool> {
    let pool = sqlx::PgPool::connect(&std::env::var("DATABASE_URL")?).await?;
    Ok(pool)
}
