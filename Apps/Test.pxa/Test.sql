-- Create script for Pylax test database
-- Thomas Führinger, 2017-12-19
;

PRAGMA application_id = 14;
PRAGMA encoding = "UTF-8";

DROP TABLE PxWindow;
CREATE TABLE PxWindow (
	PxWindowID		TEXT	PRIMARY KEY,
	Left   			INTEGER,
	Top   			INTEGER,
	Width   		INTEGER,
	Height 			INTEGER,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);

DROP TABLE PxForm;

DROP TABLE Item;
CREATE TABLE Item (
	ItemID			INTEGER	PRIMARY KEY,
	Name			TEXT    UNIQUE,
	Description		TEXT,
	Picture			BLOB,
	Price   		REAL,
	Quantity   		INTEGER,
	Expires			TIMESTAMP,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
INSERT INTO Item (ItemID, Name, Description,Price, Quantity) VALUES ( 1, 'Apple', 'Delicious fruit',12345678.123, 2761);
INSERT INTO Item (ItemID, Name, Description) VALUES ( 2, 'Orange', 'Another delicious fruit');
INSERT INTO Item (ItemID, Name) VALUES ( 3, 'Banana');

DROP TABLE ItemSold;
CREATE TABLE ItemSold (
	Item    		INTEGER,
	Customer		INTEGER,
	Quantity   		INTEGER,
	SalesDate		TIMESTAMP,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
INSERT INTO ItemSold (Item, Quantity, Customer) VALUES ( 1, 1234, 2);
INSERT INTO ItemSold (Item, Quantity, Customer) VALUES ( 1, 23456, 1);
INSERT INTO ItemSold (Item, Quantity, Customer) VALUES ( 2, 2, 2);

DROP TABLE Customer;
CREATE TABLE Customer (
	CustomerID		INTEGER	PRIMARY KEY,
	Name			TEXT    UNIQUE,
	Description		TEXT,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
INSERT INTO Customer (CustomerID, Name) VALUES ( 1, 'Waldorf');
INSERT INTO Customer (CustomerID, Name) VALUES ( 2, 'Statler');
