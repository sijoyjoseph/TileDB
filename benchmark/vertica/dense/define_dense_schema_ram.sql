CREATE TABLE dense_50000x20000_2500x1000_ram (
CID       INTEGER NOT NULL CONSTRAINT AUTO_INCREMENT PRIMARY KEY,
A1        INTEGER NOT NULL ENCODING GZIP_COMP
);

CREATE TABLE dense_50000x20000_2500x1000_ram_withrle (
CID       INTEGER NOT NULL CONSTRAINT AUTO_INCREMENT PRIMARY KEY,
A1        INTEGER NOT NULL ENCODING RLE
);