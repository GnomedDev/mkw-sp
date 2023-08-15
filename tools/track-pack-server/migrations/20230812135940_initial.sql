CREATE type TrackType AS ENUM (
    'race',
    'coin',
    'balloon'
);

CREATE TABLE users (
    discord_id bigint  PRIMARY KEY,
    device_id  integer,
    licence_id smallint,

    name       text      NOT NULL,
    pfp_hash   text      NOT NULL,
    expiry     timestamp NOT NULL,
    token_hash bytea     NOT NULL
);

CREATE TABLE packs (
    hash      bytea  PRIMARY KEY,
    author_id bigint NOT NULL,
    approved  bool   NOT NULL DEFAULT false,
    downloads bigint NOT NULL DEFAULT 0,

    name        text NOT NULL,
    description text NOT NULL,

    FOREIGN KEY (author_id) REFERENCES users (discord_id) ON DELETE CASCADE
);

CREATE TABLE pack_contents (
    pack_hash  bytea,
    track_hash bytea,
    type       TrackType,

    PRIMARY KEY (pack_hash, track_hash, type),
    FOREIGN KEY (pack_hash) REFERENCES packs (hash) ON DELETE CASCADE
);

CREATE TABLE unreleased_tracks (
    track_hash     bytea  PRIMARY KEY,
    author_id      bigint NOT NULL,
    approved       bool   NOT NULL DEFAULT false,

    track_contents bytea    NOT NULL,
    name           text     NOT NULL,
    version        text     NOT NULL,
    course_id      smallint NOT NULL,
    music_id       smallint NOT NULL,

    FOREIGN KEY (author_id) REFERENCES users (discord_id) ON DELETE CASCADE
);
