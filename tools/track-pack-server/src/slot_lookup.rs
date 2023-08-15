#[derive(Debug, Clone, Copy)]
pub enum InvalidCourseId {
    InvalidSlotKind(u8),
    InvalidCourse {
        slot_id: u8,
        is_arena: bool,
    },
}

impl std::error::Error for InvalidCourseId {}
impl std::fmt::Display for InvalidCourseId {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::InvalidSlotKind(kind) => writeln!(f, "Unknown slot kind: {kind}"),
            Self::InvalidCourse {
                slot_id,
                is_arena,
            } => {
                if *is_arena {
                    writeln!(f, "Invalid arena slot id: {slot_id}")
                } else {
                    writeln!(f, "Invalid track slot id: {slot_id}")
                }
            }
        }
    }
}

pub fn slot_to_course_id(slot_id: u8, kind: u8) -> Result<Option<u8>, InvalidCourseId> {
    if slot_id == 0 {
        Ok(None)
    } else if kind == 1 {
        slot_to_track_id(slot_id).map(Some).ok_or(InvalidCourseId::InvalidCourse {
            slot_id,
            is_arena: false,
        })
    } else if kind == 2 {
        slot_to_arena_id(slot_id).map(Some).ok_or(InvalidCourseId::InvalidCourse {
            slot_id,
            is_arena: true,
        })
    } else {
        Err(InvalidCourseId::InvalidSlotKind(kind))
    }
}

fn slot_to_track_id(slot_id: u8) -> Option<u8> {
    Some(match slot_id {
        11 => 0x8,
        12 => 0x1,
        13 => 0x2,
        14 => 0x4,
        21 => 0x0,
        22 => 0x5,
        23 => 0x6,
        24 => 0x7,
        31 => 0x9,
        32 => 0xF,
        33 => 0xB,
        34 => 0x3,
        41 => 0xE,
        42 => 0xA,
        43 => 0xC,
        44 => 0xD,
        51 => 0x10,
        52 => 0x14,
        53 => 0x19,
        54 => 0x1A,
        61 => 0x1B,
        62 => 0x1F,
        63 => 0x17,
        64 => 0x12,
        71 => 0x15,
        72 => 0x1E,
        73 => 0x1D,
        74 => 0x11,
        81 => 0x18,
        82 => 0x16,
        83 => 0x13,
        84 => 0x1C,
        _ => return None,
    })
}

fn slot_to_arena_id(slot_id: u8) -> Option<u8> {
    Some(match slot_id {
        11 => 0x21,
        12 => 0x20,
        13 => 0x23,
        14 => 0x22,
        15 => 0x24,
        21 => 0x27,
        22 => 0x28,
        23 => 0x29,
        24 => 0x25,
        25 => 0x26,
        _ => return None,
    })
}
