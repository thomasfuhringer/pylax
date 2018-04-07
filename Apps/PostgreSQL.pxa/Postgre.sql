-- Create script for Pylax test database PostgreSQL
-- Thomas Führinger, 2018-03-23

DROP TABLE Item;
CREATE TABLE Item (
	ItemID			BIGSERIAL PRIMARY KEY,
	Name			VARCHAR(70) UNIQUE,
	Description		TEXT,
	Picture			BYTEA,
	ModUser			INTEGER,
	ModDate			TIMESTAMP DEFAULT CURRENT_TIMESTAMP);

INSERT INTO Item (Name, Description) VALUES ('Apple', 'Delicious fruit');
INSERT INTO Item (Name, Description) VALUES ('Orange', 'Another delicious fruit');
INSERT INTO Item (Name) VALUES ('Banana');

GRANT SELECT, INSERT, UPDATE, DELETE ON Item TO PUBLIC;
GRANT SELECT, USAGE  ON item_itemid_seq TO PUBLIC;
