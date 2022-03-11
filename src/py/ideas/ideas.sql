/*
 * Author: fasion
 * Created time: 2022-03-07 18:13:04
 * Last Modified by: fasion
 * Last Modified time: 2022-03-07 18:33:00
 */

DROP TABLE IF EXISTS Users;
CREATE TABLE Users (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	username TEXT,
	userpass TEXT,
	coins INTEGER
);
INSERT INTO Users VALUES
	(0, "admin", "admin123", 100),
	(2, "alice", "123456", 100),
	(3, "fasion", "123abc", 100),
	(4, "tom", "mot", 100);

DROP TABLE IF EXISTS Ideas;
CREATE TABLE Ideas (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	content TEXT,
	introducer_id INTEGER,
	introduced_ts INTEGER
);
INSERT INTO Ideas VALUES
	(0, "欢迎来到点子广场！", 0, 1646562745);
