macro_rules! forward_fmt {
    ($impl_on:ident) => {
        impl std::fmt::Display for $impl_on {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                self.0.fmt(f)
            }
        }
    };
}

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Debug)]
pub struct RoomId(pub u16);

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Debug)]
pub struct GameserverID(pub u16);

forward_fmt!(RoomId);
forward_fmt!(GameserverID);
