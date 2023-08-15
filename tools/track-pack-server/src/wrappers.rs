use crate::Result;
use netprotocol::pack_serving::ProtoSha1;
use sha1::Digest;

#[derive(
    Hash,
    Default,
    Clone,
    Copy,
    PartialEq,
    Eq,
    PartialOrd,
    Ord,
    serde_with::DeserializeFromStr,
    serde_with::SerializeDisplay,
)]
pub struct Sha1([u8; 0x14]);

impl Sha1 {
    pub fn into_inner(self) -> [u8; 0x14] {
        self.0
    }

    pub fn hash(data: &[u8]) -> Self {
        let output = sha1::Sha1::digest(data);
        let output_ref: &[u8] = output.as_ref();
        Self(output_ref.try_into().expect("Sha1's should always fit in 0x14 bytes"))
    }
}

impl std::fmt::Display for Sha1 {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let mut encoded = [0_u8; 0x28];
        hex::encode_to_slice(self.0, &mut encoded).expect("Sha1 not containing valid hex");

        let encoded_str = std::str::from_utf8(&encoded).expect("Hex is always valid UTF8");
        f.write_str(encoded_str)
    }
}

impl From<[u8; 0x14]> for Sha1 {
    fn from(value: [u8; 0x14]) -> Self {
        Self(value)
    }
}

impl<'a> From<&'a Sha1> for ProtoSha1 {
    fn from(value: &'a Sha1) -> ProtoSha1 {
        ProtoSha1 {
            data: value.0.to_vec(),
        }
    }
}

impl TryFrom<ProtoSha1> for Sha1 {
    type Error = anyhow::Error;

    fn try_from(value: ProtoSha1) -> std::result::Result<Self, Self::Error> {
        let err = |_| anyhow::anyhow!("Invalid length for Sha1");
        value.data.try_into().map(Self).map_err(err)
    }
}

impl std::str::FromStr for Sha1 {
    type Err = anyhow::Error;

    fn from_str(encoded: &str) -> std::result::Result<Self, Self::Err> {
        let mut decoded = [0_u8; 0x14];
        hex::decode_to_slice(encoded, &mut decoded)?;
        Ok(Self(decoded))
    }
}

impl std::fmt::Debug for Sha1 {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Sha1({self})")
    }
}

async fn fetch(reqwest: &reqwest::Client, url: impl reqwest::IntoUrl) -> Result<reqwest::Response> {
    reqwest.get(url).send().await?.error_for_status().map_err(Into::into)
}

pub async fn fetch_text(reqwest: &reqwest::Client, url: impl reqwest::IntoUrl) -> Result<String> {
    fetch(reqwest, url).await?.text().await.map_err(Into::into)
}

pub async fn fetch_bytes(reqwest: &reqwest::Client, url: impl reqwest::IntoUrl) -> Result<Vec<u8>> {
    Ok(fetch(reqwest, url).await?.bytes().await?.to_vec())
}
