#!/usr/bin/env python
# -*- encoding=utf8 -*-

'''
Author: fasion
Created time: 2022-03-07 17:54:32
Last Modified by: fasion
Last Modified time: 2022-03-11 17:36:54
'''

import functools
import sqlite3
import threading
import traceback
import time

from flask import (
	Flask,
	jsonify,
	redirect,
	render_template,
	request,
	session,
)

app = Flask(__name__)
app.secret_key = b'1234567890'

def dict_factory(cursor, row):
	d = {}
	for i, col in enumerate(cursor.description):
		d[col[0]] = row[i]
	return d

thread_locals = threading.local()
def getdb():
	db = getattr(thread_locals, 'db', None)
	if db:
		return db

	thread_locals.db = db = sqlite3.connect('ideas.db')
	db.row_factory = dict_factory

	return db

def catch_exception(handler):
	@functools.wraps(handler)
	def proxy(*args, **kwargs):
		try:
			return handler(*args, **kwargs)
		except Exception as e:
			msg = traceback.format_exc()
			print(msg)
			return render_template('message.html', msg=msg)
	return proxy

def ensure_db_cursor(handler):
	@functools.wraps(handler)
	def proxy(*args, **kwargs):
		db = getdb()
		cursor = db.cursor()
		try:
			return handler(*args, **kwargs, db=db, cursor=cursor)
		finally:
			cursor.close()

	return proxy

def ensure_login(handler):
	@functools.wraps(handler)
	def proxy(db, cursor, *args, **kwargs):
		user_id = session.get('user_id')
		if not user_id:
			return redirect('/login')

		cursor.execute('SELECT * FROM Users where id=?', (user_id,))
		for user in cursor.fetchall():
			return handler(*args, current_user=user, db=db, cursor=cursor, **kwargs)

		return redirect('/login')

	return proxy

@app.route("/login", methods=['GET', 'POST'])
@catch_exception
@ensure_db_cursor
def login(db, cursor):
	msg = ''
	if request.method == 'POST':
		username = request.form['username']
		userpass = request.form['userpass']

		sql = 'SELECT * FROM Users where username="{}" and userpass="{}"'.format(
			username,
			userpass,
		)

		cursor.execute(sql)
		for user in cursor.fetchall():
			session['user_id'] = user['id']
			return redirect('/')
		else:
			msg = 'username or userpass is wrong!'

	return render_template("login.html", msg=msg)

@app.route("/logout", methods=['GET', 'POST'])
@catch_exception
@ensure_db_cursor
@ensure_login
def logout(current_user, db, cursor):
	session.pop('user_id')
	return redirect('/')

@app.route("/", methods=['GET', 'POST'])
@catch_exception
@ensure_db_cursor
@ensure_login
def home(current_user, db, cursor):
	if request.method == 'POST':
		cursor.execute('INSERT INTO Ideas (content, introducer_id, introduced_ts) values (?, ?, ?)', (
			request.form['content'],
			session['userid'],
			int(time.time()),
		))
		db.commit()
		return redirect('/')

	cursor.execute('''SELECT Ideas.*, username as introducer_username FROM Ideas LEFT JOIN Users Where Ideas.introducer_id=Users.id ORDER BY introduced_ts DESC''')
	ideas = cursor.fetchall()

	return render_template('home.txt', current_user=current_user, ideas=ideas)

@app.route("/rank", methods=['GET'])
@catch_exception
@ensure_db_cursor
@ensure_login
def rank(current_user, db, cursor):
	cursor.execute('SELECT * FROM Users ORDER BY coins DESC')
	users = cursor.fetchall()
	return render_template('rank.html', current_user=current_user, users=users)

@app.route("/delete-idea/<idea_id>", methods=['POST'])
@catch_exception
@ensure_db_cursor
@ensure_login
def delete_idea(idea_id, current_user, db, cursor):
	cursor.execute('SELECT * FROM Ideas WHERE id=? AND introducer_id=?', (int(idea_id), session['userid']))
	if not cursor.fetchall():
		return render_template('message.html', msg='idea not found or not yours')

	if cursor.execute('DELETE FROM Ideas WHERE id=?', (int(idea_id), )).rowcount:
		db.commit()
		return redirect('/')
	else:
		return render_template('message.html', msg='idea not found')

@app.route("/reward", methods=['GET', 'POST'])
@catch_exception
@ensure_db_cursor
@ensure_login
def reward(current_user, db, cursor):
	receiver_id = request.args.get('receiver_id')
	if not receiver_id:
		return render_template('message.html', msg='bad receiver id: {}'.format(receiver_id))

	cursor.execute('SELECT * FROM Users WHERE id=?', (int(receiver_id),))
	receivers = cursor.fetchall()
	if not receivers:
		return render_template('message.html', msg='bad receiver id: {}'.format(receiver_id))

	receiver = receivers[0]

	if request.method == 'GET':
		return render_template('reward.html', receiver=receiver)

	coins = int(request.form.get('coins'))
	if coins <= 0:
		return render_template('reward.html', receiver=receiver, msg="Coins not valid!")

	if current_user['coins'] < coins:
		return render_template('reward.html', receiver=receiver, msg="coins not enough")

	result = cursor.execute('UPDATE Users Set coins=coins-? WHERE id=? AND coins>=?', (coins, current_user['id'], coins))
	if not result.rowcount:
		return render_template('reward.html', receiver=receiver, msg="coins not enough")

	cursor.execute('UPDATE Users Set coins=coins+? WHERE id=?', (coins, receiver['id']))
	if not result.rowcount:
		return render_template('reward.html', receiver=receiver, msg="coin transfer failed")

	db.commit()

	return render_template('reward.html', receiver=receiver, msg="{} coins transfer to {} successfully".format(coins, receiver['id']))
