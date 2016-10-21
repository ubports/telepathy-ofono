CREATE TABLE mms_groups (
    groupId varchar(255) PRIMARY KEY,
    subject varchar(512)
);

CREATE TABLE mms_group_members (
    groupId varchar(255),
    memberId varchar(255),
    FOREIGN KEY(groupId) REFERENCES mms_groups(groupId)
);
