use std::sync::atomic::{AtomicU32, Ordering};

pub use crate::inner::{client_id::Inner as ClientIdRaw, ClientId as ClientIdOpt, LoggedInId};

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Debug)]
pub enum ClientId {
    LoggedIn {
        device: u32,
        licence: u16,
    },
    Guest {
        id: u32,
    },
}

impl ClientId {
    pub fn new(login: Option<(u32, u16)>) -> Self {
        static GUEST_ID: AtomicU32 = AtomicU32::new(0);
        match login {
            Some((device, licence)) => Self::LoggedIn {
                device,
                licence,
            },
            None => Self::Guest {
                id: GUEST_ID.fetch_add(1, Ordering::Relaxed),
            },
        }
    }
}

impl std::fmt::Display for ClientId {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::LoggedIn {
                device,
                licence,
            } => write!(f, "LoggedIn({device}, {licence})"),
            Self::Guest {
                id,
            } => write!(f, "Guest({id})"),
        }
    }
}

impl From<ClientIdOpt> for ClientId {
    fn from(id: ClientIdOpt) -> Self {
        let id = id.inner.expect("Invalid client ID");

        match id {
            ClientIdRaw::LoggedIn(LoggedInId {
                device,
                licence,
            }) => Self::LoggedIn {
                device,
                licence: licence as u16,
            },
            ClientIdRaw::Guest(id) => Self::Guest {
                id,
            },
        }
    }
}

impl From<ClientId> for ClientIdRaw {
    fn from(id: ClientId) -> Self {
        match id {
            ClientId::LoggedIn {
                device,
                licence,
            } => Self::LoggedIn(LoggedInId {
                device,
                licence: licence as u32,
            }),
            ClientId::Guest {
                id,
            } => Self::Guest(id),
        }
    }
}
