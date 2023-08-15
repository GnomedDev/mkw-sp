pub mod async_stream;

mod client_id;
mod negotiation;

#[cfg(any(feature = "online_gameplay", feature = "pack_downloading"))]
pub use client_id::*;
pub use negotiation::*;

mod inner {
    include!(concat!(env!("OUT_DIR"), "/_.rs"));
}

#[cfg(feature = "online_gameplay")]
pub mod room_protocol {
    pub use super::inner::{
        room_event, room_event::Event as RoomEvent, room_request,
        room_request::Request as RoomRequest, ClientId as ClientIdOpt, LoginInfo,
        RoomEvent as RoomEventOpt, RoomRequest as RoomRequestOpt,
    };
    pub use super::inner::{RaceClientPing, RaceServerFrame};
}

#[cfg(feature = "online_gameplay")]
pub mod matchmaking {
    pub use super::inner::{
        cts_message, cts_message::Message as CTSMessage, gts_message,
        gts_message::Message as GTSMessage, stc_message, stc_message::Message as STCMessage,
        stg_message, stg_message::Message as STGMessage, ClientId as ClientIdOpt,
        CtsMessage as CTSMessageOpt, GtsMessage as GTSMessageOpt, LoggedInId, LoginInfo,
        StcMessage as STCMessageOpt, StgMessage as STGMessageOpt,
    };
}

#[cfg(feature = "pack_downloading")]
pub mod pack_serving {
    pub use super::inner::{
        ctts_message, ctts_message::Message as CTTSMessage, tstc_message,
        tstc_message::Message as TSTCMessage, CttsMessage as CTTSMessageOpt, Pack, ProtoSha1,
        ProtoTrack, TstcMessage as TSTCMessageOpt,
    };
}

#[cfg(feature = "update")]
pub mod update_server {
    pub use super::inner::{UpdateRequest, UpdateResponse};
}
