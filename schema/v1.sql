CREATE TABLE schema_version (
    version int
);

CREATE TABLE pending_messages (
    messageId varchar(255),
    recipientId varchar(255),
    timestamp datetime
);
