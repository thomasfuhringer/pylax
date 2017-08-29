-- Create script for Hinterland
-- Thomas Führinger, 2017-03-16
-- modified 2017-08-29
;

PRAGMA application_id = 7;
PRAGMA encoding = "UTF-8";

DROP TABLE Person;
CREATE TABLE Person (
	PersonID		INTEGER	PRIMARY KEY,
	FirstName		TEXT,
	MiddleName		TEXT,
	LastName		TEXT,
	Transcription	TEXT,
	EMail			TEXT,
	Phone			TEXT,
	Picture			BLOB,
	PublicKey		BLOB,
	Handle			TEXT COLLATE NOCASE UNIQUE,
	Language    	INTEGER,          -- ISO 3166-1
	Location		BLOB,
	Password		BLOB,
	PasswordSalt	BLOB,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
INSERT INTO Person (PersonID, FirstName, LastName, Handle) VALUES (0, 'Main', 'Administrator', 'Admin');

DROP TABLE Org;
CREATE TABLE Org (
	OrgID			INTEGER	PRIMARY KEY,
	Parent			INTEGER,
	Name			TEXT,
	Description		TEXT,
	Logo			BLOB,
	HostName		TEXT,
	HostPort		INTEGER,
	PublicKey		BLOB,
	Location		BLOB,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (Parent) REFERENCES Page(OrgID));
INSERT INTO Org (OrgID, Name, HostName, HostPort) VALUES (1, 'Hinterland Center', '45.76.133.182', 1550);

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

DROP TABLE Role;
CREATE TABLE Role (
	Org			    INTEGER,
	Person			INTEGER,
	Type			INTEGER DEFAULT 0,
	Title			TEXT,
	Public			INTEGER DEFAULT 0,
	Administrator	INTEGER DEFAULT 0,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
CREATE UNIQUE INDEX RolePK ON Role (Org, Person);
INSERT INTO Role (Org, Person, Type, Administrator) VALUES (NULL, 0, 0, 1); -- Administrator everywhere

DROP TABLE Page;
CREATE TABLE Page (
	PageID			INTEGER	PRIMARY KEY,
	KeyWord	     	TEXT COLLATE NOCASE,
	Org			    INTEGER,
	Person			INTEGER,
	Heading	    	TEXT,
	Text	    	TEXT,
	Data			BLOB,
	Tags	    	TEXT,
	Language    	INTEGER,
	Parent			INTEGER,
	Views			INTEGER DEFAULT 0,
	LastView		TIMESTAMP,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (Parent) REFERENCES Page(PageID));
CREATE UNIQUE INDEX PageUK ON Page (KeyWord, Parent, Org, Person, Language);
INSERT INTO Page (KeyWord, Text) VALUES (NULL, '### Welcome to Hinterland!');


DROP TABLE Message;
CREATE TABLE Message (
	MessageID		INTEGER	PRIMARY KEY,
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
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (Parent) REFERENCES Page(MessageID));


DROP TABLE Addressee;
CREATE TABLE Addressee (
	Message 		INTEGER,
	Person			INTEGER,
	Org			    INTEGER,
	Delivered		TIMESTAMP,
	Read     		TIMESTAMP);
CREATE UNIQUE INDEX AddresseePK ON Addressee (Message, Person, Org);


DROP TABLE Log;
CREATE TABLE Log (
	Level			INTEGER,
	Session			INTEGER,
	Message			TEXT,
	User        	INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);

DROP TABLE Item;
CREATE TABLE Item (
	ItemID			INTEGER	PRIMARY KEY,
	Name			TEXT    UNIQUE,
	Description		TEXT,
	Picture			BLOB,
	Parent			INTEGER,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
INSERT INTO Item (ItemID, Name, Description) VALUES ( 1, 'Apple', 'Delicious fruit');
INSERT INTO Item (ItemID, Name, Description) VALUES ( 2, 'Orange', 'Another delicious fruit');
