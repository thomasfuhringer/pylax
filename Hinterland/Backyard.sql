-- Create script for Hinterland Backyard
-- Thomas Führinger, 2017-06-17
;

PRAGMA application_id = 7;
PRAGMA encoding = "UTF-8";

DROP TABLE Site;

CREATE TABLE Site (
	SiteID			INTEGER	PRIMARY KEY,
	Name			TEXT UNIQUE,
	Description		TEXT,
	Logo			BLOB,
	HostName		TEXT,
	HostPort		INTEGER,
	PublicKey		BLOB,
	PrivateKey		BLOB, -- own
	Me           	INTEGER,
	LastConnection	TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
CREATE UNIQUE INDEX SitePK ON Site (HostName, HostPort);
INSERT INTO Site (SiteID, Name, HostName, HostPort) VALUES (1, 'Hinterland Center', '45.76.133.182', 1550);

DROP TABLE Person;
CREATE TABLE Person (
	Site			INTEGER,
	PersonID		INTEGER,
	FirstName		TEXT,
	MiddleName		TEXT,
	LastName		TEXT,
	Transcription	TEXT,
	EMail			TEXT,
	Phone			TEXT,
	Picture			BLOB,
	Password		BLOB,
	PrivateKey		BLOB,
	PublicKey		BLOB,
	Handle			TEXT COLLATE NOCASE,
	Language    	INTEGER,          -- ISO 3166-1
	Location		BLOB,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
CREATE UNIQUE INDEX PersonPK ON Person (Site, PersonID);

DROP TABLE Enum;
CREATE TABLE Enum ( -- Type 0: general (1: token), Type 1: Role types, 2: encryption keys (0: public, 1: private)
	Type 	        INTEGER,
	Item		    INTEGER,
	Name			TEXT,
	ModUser			INTEGER DEFAULT 0,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
CREATE UNIQUE INDEX EnumPK ON Enum (Type, Item);

INSERT INTO Enum (Type, Item, Name) VALUES (0, 0, '0');
INSERT INTO Enum (Type, Item, Name) VALUES (0, 1, NULL);
INSERT INTO Enum (Type, Item, Name) VALUES (1, 0, 'Observer');
INSERT INTO Enum (Type, Item, Name) VALUES (1, 10, 'Member');
INSERT INTO Enum (Type, Item, Name) VALUES (1, 20, 'Employee');
INSERT INTO Enum (Type, Item, Name) VALUES (1, 50, 'Manager');
INSERT INTO Enum (Type, Item, Name) VALUES (2, 0, NULL);
INSERT INTO Enum (Type, Item, Name) VALUES (2, 1, NULL);


DROP TABLE Message;
CREATE TABLE Message (
	Site			INTEGER,
	MessageID		INTEGER,
	Person	        INTEGER NOT NULL,
	Org		        INTEGER,
	Heading	    	TEXT,
	Text	    	TEXT,
	Data			BLOB,
	Encrypted		INTEGER DEFAULT 0,
	Page			INTEGER,
	Tags	    	TEXT,
	Parent			INTEGER,
	Location		BLOB,
	ModUser			INTEGER DEFAULT 0,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
CREATE UNIQUE INDEX MessagePK ON Message (Site, MessageID);


DROP TABLE Addressee;
CREATE TABLE Addressee (
	Site			INTEGER,
	Message 		INTEGER,
	Person			INTEGER,
	Org			    INTEGER,
	Delivered		TIMESTAMP,
	Read     		TIMESTAMP);
CREATE UNIQUE INDEX AddresseePK ON Addressee (Site, Message, Person, Org);
