#!/usr/bin/env python2
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# Copyright (C) 2013 Luka Perkov <freeacs-ng@lukaperkov.net>
#

import copy
import datetime
import json
import logging
import time

import pymongo
from pymongo import MongoClient
from bson.objectid import ObjectId

from provisioning import Dictionary


def replace_keys(dict_old, old, new):
	dict_new = Dictionary()

	for k,v in dict_old.items():
		dict_new[k.replace(old, new)] = v

	return dict_new

class DatabaseMongoDB(object):
	LOG = logging.getLogger(__name__)

	def __init__(self):
		self._connection = None
		self._db = None
		self._collection = None

		self.config = Dictionary()

		self.config["mongo"]["host"] = "localhost"
		self.config["mongo"]["port"] = 27017
		self.config["mongo"]["max_pool_size"] = 10
		self.config["mongo"]["db"] = None
		self.config["mongo"]["collection"] = None
		self.config["mongo"]["ttl"] = None
		self.config["config"]["log"]["level"] = logging.INFO
		self.config["config"]["log"]["format"] = "%(asctime)s  %(funcName) -40s: %(message)s"

	def configure(self):
		logging.basicConfig(level=self.config["config"]["log"]["level"],
				    format=self.config["config"]["log"]["format"])

		self.LOG.info('freeacs-ng python database mongo backend initialized')

	def connect(self):
		if self.config["mongo"]["db"] == None:
			self.LOG.error('mongo database is not configured')
			raise SystemExit

		if self.config["mongo"]["collection"] == None:
			self.LOG.error('mongo collection is not configured')
			raise SystemExit

		try:
			self._connection = MongoClient(host=self.config["mongo"]["host"],
						       port=self.config["mongo"]["port"],
						       max_pool_size=self.config["mongo"]["max_pool_size"])
 		except pymongo.errors.ConfigurationError:
			self.LOG.error('mongo raised configuration error exception')
			raise SystemExit
 		except pymongo.errors.ConnectionFailure:
			self.LOG.error('mongo raised connection failure exception')
			raise SystemExit
 		except pymongo.errors.InvalidURI:
			self.LOG.error('mongo raised invalid uri exception')
			raise SystemExit
 		except pymongo.errors.TimeoutError:
			self.LOG.error('mongo raised timeout error exception')
			raise SystemExit
 		except:
			self.LOG.error('mongo raised exception')
			raise SystemExit

		try:
			self._db = self._connection[self.config["mongo"]["db"]]
		except:
			self.LOG.error('connecting to database failed')
			raise SystemExit

		try:
			self._collection = self._db[self.config["mongo"]["collection"]]
		except:
			self.LOG.error('connecting to collection failed')
			raise SystemExit

		try:
			if type(self.config["mongo"]["ttl"]) is int:
				self._collection.ensure_index(key_or_list=[("_time", 1)],
							      name="_time_ttl_idx",
							      expireAfterSeconds=self.config["mongo"]["ttl"])
		except:
			self.LOG.error('creating index failed')
			raise SystemExit

		self.LOG.info('connection open')

	def insert(self, data):
		self.LOG.info('inserting data into mongo collection')

		mongo_obj = copy.deepcopy(data)

		isodate = datetime.datetime.utcfromtimestamp(time.time())
		mongo_obj["_time"] = isodate

 		try:
 			d = replace_keys(data["cwmp"]["parameters"], '.', '*')
			mongo_obj["cwmp"]["parameters"] = d
 		except:
 			pass

 		try:
 			mongo_obj_id = self._collection.insert(mongo_obj)

 		except pymongo.errors.AutoReconnect:
			self.LOG.error('mongo raised auto reconnect exception')
			raise SystemExit
 		except pymongo.errors.TimeoutError:
			self.LOG.error('mongo raised timeout error exception')
			raise SystemExit
 		except:
			self.LOG.error('mongo raised exception')
			raise SystemExit

		self.LOG.info('data inserted into mongo collection')
		return mongo_obj_id

	def find_one(self, data):
		self.LOG.info('finding data in mongo collection')

 		try:
 			mongo_obj = self._collection.find_one()

 		except pymongo.errors.AutoReconnect:
			self.LOG.error('mongo raised auto reconnect exception')
			raise SystemExit
 		except pymongo.errors.TimeoutError:
			self.LOG.error('mongo raised timeout error exception')
			raise SystemExit
 		except:
			self.LOG.error('mongo raised exception')
			raise SystemExit

 		try:
 			d = replace_keys(mongo_obj["cwmp"]["parameters"], '*', '.')
			mongo_obj["cwmp"]["parameters"] = d
 		except:
 			pass

		self.LOG.info('data found in mongo collection')
		return mongo_obj

