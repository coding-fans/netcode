/*
 * Author: fasion
 * Created time: 2022-03-07 18:13:04
 * Last Modified by: fasion
 * Last Modified time: 2022-03-12 17:28:28
 */

DROP TABLE IF EXISTS Users;
CREATE TABLE Users (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	username TEXT,
	userpass TEXT,
	coins INTEGER
);
INSERT INTO Users VALUES
	(1, "admin", "admin123", 100),
	(2, "fasion", "fasion123", 100),
	(3, "alan", "alan123", 100),
	(4, "alice", "alice123", 100),
	(5, "bill", "bill123", 100),
	(6, "david", "david123", 100),
	(7, "eric", "eric123", 100),
	(8, "henry", "henry123", 100),
	(9, "jack", "jack123", 100),
	(10, "jim", "jim123", 100),
	(11, "kate", "kate123", 100),
	(12, "lily", "lily123", 100),
	(13, "lucy", "lucy123", 100),
	(14, "mike", "mike123", 100),
	(15, "paul", "paul123", 100),
	(16, "tina", "tina123", 100),
	(17, "tom", "tom123", 100),
	(18, "z", "z123", 100);

DROP TABLE IF EXISTS Ideas;
CREATE TABLE Ideas (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	content TEXT,
	introducer_id INTEGER,
	introduced_ts INTEGER
);
INSERT INTO Ideas VALUES
	(1, "You can post your wonderful ideas on IdeaHub!", 1, 1646562745);
