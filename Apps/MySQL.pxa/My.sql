-- Create script for Pylax test database MySQL
-- Thomas Führinger, 2018-04-12

DROP TABLE Item;
CREATE TABLE Item (
	ItemID          SERIAL PRIMARY KEY,
	Name            VARCHAR(70) UNIQUE,
	Description     TEXT,
	Picture         BLOB,
	ModUser         INTEGER,
	ModDate         TIMESTAMP DEFAULT CURRENT_TIMESTAMP);

INSERT INTO Item (Name, Description) VALUES ('Apple', 'Delicious fruit');
INSERT INTO Item (Name, Description) VALUES ('Orange', 'Another delicious fruit');
INSERT INTO Item (Name) VALUES ('Banana');

GRANT ALL ON Item.* TO ''@'localhost'
--GRANT SELECT, USAGE  ON item_itemid_seq TO PUBLIC;
