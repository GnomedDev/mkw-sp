use std::{io::ErrorKind, str::FromStr, sync::OnceLock};

use anyhow::{bail, Error, Result};
use arrayvec::ArrayVec;
use dashmap::DashMap;
use netprotocol::pack_serving::{ProtoSha1, ProtoTrack};

use crate::{
    slot_lookup::slot_to_course_id,
    wrappers::{fetch_bytes, fetch_text, Sha1},
};

const DOWNLOAD_URL_RE_RAW: &str = r"href=/d/(@.+)/([0-9]+)";
static DOWNLOAD_URL_RE: OnceLock<regex::Regex> = OnceLock::new();

#[derive(sqlx::FromRow)]
struct UnreleasedTrack {
    name: String,
    version: String,
    course_id: i32,
    music_id: i32,
    track_contents: Vec<u8>,
}

impl From<UnreleasedTrack> for ProtoTrack {
    fn from(t: UnreleasedTrack) -> Self {
        Self {
            name: t.name,
            course_id: t.course_id as u32,
            music_id: Some(t.music_id as u32),
            version: Some(t.version),
            wiimm_id: None,
        }
    }
}

pub type TrackDB = DashMap<Sha1, ProtoTrack>;

pub fn is_battle(course_id: i16) -> bool {
    course_id >= 0x20
}

fn handle_pre_version(version: &str, to_strip: &str, key: i16) -> Option<i16> {
    let Some(without_alpha) = version.strip_prefix(to_strip) else {
        return None;
    };

    let mut offset = 0;
    if let Ok(ver) = i8::from_str(without_alpha) {
        offset = ver as i16;
    }

    Some(offset + key)
}

pub fn extract_version(version: &str) -> i16 {
    let version = version.to_ascii_lowercase();
    let pre_version = handle_pre_version(&version, "pre", -3)
        .or_else(|| handle_pre_version(&version, "alpha", -2))
        .or_else(|| handle_pre_version(&version, "beta", -1));

    if let Some(ver) = pre_version {
        return ver;
    }

    // Strip off the v prefix
    let version = version.strip_prefix('v').unwrap_or(&version);

    // Put .beta or .opt versions at the end of the list
    let mut version_iter = version.split('.');

    if let Some(Ok(ver)) = version_iter.next().map(i8::from_str) {
        let mut key = (ver as i16) * 10;
        if let Some(Ok(ver_dec)) = version_iter.next().map(i8::from_str) {
            key += ver_dec as i16;
        }

        if version_iter.next().is_some() {
            key += 1;
        }

        key
    } else {
        i16::MAX
    }
}

async fn get_download_url(reqwest: &reqwest::Client, wiimm_id: u32) -> Result<String> {
    let track_page = fetch_text(reqwest, format!("https://ct.wiimm.de/i/{wiimm_id}")).await?;

    let re = DOWNLOAD_URL_RE.get_or_init(|| regex::Regex::new(DOWNLOAD_URL_RE_RAW).unwrap());
    let captures = re.captures_iter(&track_page);

    for (_, [cookie_id, id]) in captures.map(|m| m.extract()) {
        let id: u32 = id.parse()?;
        if wiimm_id == id {
            return Ok(format!("https://ct.wiimm.de/dl/{cookie_id}/{wiimm_id}"));
        }
    }

    bail!("Could not find track download link!");
}

/// Loads a track file from either:
/// - local cache stored at `tracks/id.wbz`
/// - wiiimm's track DB, via scraping the download URL
/// - the postgres database, for unreleased tracks
pub async fn load_track_file(
    reqwest: &reqwest::Client,
    track_db: &TrackDB,
    pack_db: &sqlx::PgPool,
    user_id: i64,
    track_id: ProtoSha1,
) -> Result<Option<(ProtoTrack, Vec<u8>)>> {
    let track_id = track_id.try_into()?;
    let Some(track) = track_db.get(&track_id).map(|t| t.clone()) else {
        return Ok(None);
    };

    let path = format!("tracks/{track_id}.wbz");
    match tokio::fs::read(path).await {
        Ok(cached) => return Ok(Some((track, cached))),
        Err(err) if err.kind() != ErrorKind::NotFound => return Err(err.into()),
        Err(_) => {}
    };

    if let Some(wiimm_id) = track.wiimm_id {
        let download_url = get_download_url(reqwest, wiimm_id).await?;
        let track_file = fetch_bytes(reqwest, download_url).await?;
        Ok(Some((track, track_file)))
    } else {
        let query = sqlx::query_as::<_, UnreleasedTrack>(
            "SELECT * FROM unreleased_tracks
            WHERE track_hash = $1 AND (approved = true OR author_id = $2)",
        );

        let Some(mut unreleased_track) =
            query.bind(track_id.into_inner()).bind(user_id).fetch_optional(pack_db).await?
        else {
            return Ok(None);
        };

        let contents = std::mem::take(&mut unreleased_track.track_contents);
        let track = unreleased_track.into();
        Ok(Some((track, contents)))
    }
}

async fn get_track_database(reqwest: &reqwest::Client) -> Result<String> {
    const SECS_IN_DAY: u64 = 60 * 60 * 24;

    if let Ok(metadata) = tokio::fs::metadata("public-ref.list").await {
        if metadata
            .modified()
            .map_err(Error::from)
            .and_then(|m| m.elapsed().map_err(Into::into))
            .map_or(false, |m| m.as_secs() <= SECS_IN_DAY)
        {
            return Ok(tokio::fs::read_to_string("public-ref.list").await?);
        }
    }

    let public_ref = fetch_text(reqwest, "http://archive.tock.eu/wbz/public-ref.list").await?;
    tokio::fs::write("public-ref.list", &public_ref).await?;
    Ok(public_ref)
}

pub async fn parse(reqwest: &reqwest::Client) -> Result<TrackDB> {
    let public_ref = get_track_database(reqwest).await?;
    let extended_list = tokio::fs::read_to_string("extended-tracks.csv").await?;

    let mut max_length = 0;

    let track_db = TrackDB::new();
    for line in public_ref.split('\n').skip(1).chain(extended_list.split('\n').skip(1)) {
        // sha1, wiimmId, _, _, ctype, slot, music_slot, _, _, prefix, name, _ , version, _, _, _
        // 0   , 1      , 2, 3, 4    , 5   , 6         , 7, 8, 9     , 10  , 11, 12     ,13,14,15
        if line.trim() == "" {
            continue;
        }

        let line_split: ArrayVec<&str, 16> = line.split('|').collect();
        let sha1 = Sha1::from_str(line_split[0])?;

        if line_split.len() <= 10 {
            bail!("Track DB source line too short!");
        }

        let Some(course_id) = slot_to_course_id(line_split[5].parse()?, line_split[4].parse()?)?
        else {
            continue;
        };

        max_length = line_split[12].len().max(max_length);
        let track = ProtoTrack {
            course_id: course_id as u32,
            wiimm_id: Some(line_split[1].parse()?),
            version: Some(line_split[12].to_owned()),
            music_id: (line_split[6] != "0").then(|| line_split[6].parse()).transpose()?,
            name: {
                let prefix = line_split[9];
                let name = line_split[10];

                let mut combined_name = String::with_capacity(prefix.len() + 1 + name.len());
                combined_name.push_str(prefix);
                if !prefix.is_empty() {
                    combined_name.push(' ');
                }
                combined_name.push_str(name);
                combined_name
            },
        };

        track_db.insert(sha1, track);
    }

    tracing::info!("Loaded {} tracks into the track DB", track_db.len());
    Ok(track_db)
}

#[cfg(test)]
mod test {
    use std::future::Future;

    use sha1::Digest;

    use super::{fetch_bytes, get_download_url};

    fn block_on<T>(fut: impl Future<Output = T>) -> T {
        tracing_subscriber::fmt::try_init().ok();
        tokio::runtime::Builder::new_multi_thread().enable_all().build().unwrap().block_on(fut)
    }

    #[test]
    fn test_track_db() {
        block_on(async {
            let reqwest = reqwest::Client::new();
            let track_db = super::parse(&reqwest).await.unwrap();

            let normal_db = "6dea567e6b20e8766215613b65885a652c9de88d".parse().unwrap();
            assert_eq!(track_db.get(&normal_db).unwrap().name, "Aquadrom Stage");

            let extended_db = "7e293e74991b0bf33e2ffa420b2ebe735ed23c38".parse().unwrap();
            assert_eq!(track_db.get(&extended_db).unwrap().name, "Six King Labyrinth");
        });
    }

    #[test]
    fn test_download_track() {
        block_on(async {
            let reqwest = reqwest::Client::new();
            let wiimm_id = 6165;

            let url = get_download_url(&reqwest, wiimm_id).await.unwrap();
            assert!(url.contains("https://"));
            assert!(url.contains("6165"));

            let file = fetch_bytes(&reqwest, url).await.unwrap();
            assert_eq!(
                "53a004bd5d2fd3242437edc5c97cf1b424be4b80",
                hex::encode(sha1::Sha1::digest(file))
            );
        });
    }
}
