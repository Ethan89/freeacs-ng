#!/usr/bin/env python2
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# Copyright (C) 2013 Luka Perkov <freeacs-ng@lukaperkov.net>
#

# core
import copy
import base64
import random

# mongo
from bson.objectid import ObjectId

# backend
from provisioning import *
from database_mongodb import *

class WIFIProvisioning(Provisioning):
	def action(self, data):

		if not data["cwmp"]["status"] == "0":
			self.LOG.error('add object rpc failed, ignoring message')
			return

		mongo_msg = self.database_mongodb.find_one({ "_id": ObjectId(data["cwmp"]["id"]) })

		try:
			if not mongo_msg["relay"]["id"] == "wifi_provisioning_1_add_object":
				self.LOG.info('message contains different relay id, ignoring message')
				return
		except:
			self.LOG.error('invalid json structure was stored in mongo, ignoring message')
			return

		amqp_msg = [ ]
		for i in mongo_msg["relay"]["amqp"]:
			m = base64.b64decode(i).replace(".{i}.", ".%s." % (data["cwmp"]["instance_number"]))
			self.LOG.info('publishing message with content\n%s', m)

			d = json.loads(m)
			amqp_msg.append(d)

		self.start_publishing( amqp_msg )

def main():
	provisioning = WIFIProvisioning()

	provisioning.config["amqp"]["host"] = "127.0.0.1"
	provisioning.config["amqp"]["virtual_host"] = "/"
	provisioning.config["amqp"]["credentials"]["username"] = "guest"
	provisioning.config["amqp"]["credentials"]["password"] = "guest"
	provisioning.config["amqp"]["connection_attempts"] = 100
	provisioning.config["amqp"]["retry_delay"] = 5

	provisioning.consumer["internal"]["queue"]["name"] = "wifi_provisioning_1_set_parameter_values"
	provisioning.consumer["internal"]["queue"]["arguments"] = { 'x-expires': 60 * 10 * 1000 }

	provisioning.input_verification["http"]["remote_addr"] = True
	provisioning.input_verification["http"]["authorization"]["username"] = True
	provisioning.input_verification["http"]["authorization"]["password"] = True
	provisioning.input_verification["http"]["cookie_level"] = False

	provisioning.input_verification["cwmp"]["type"]["add_object_response"] = True
	provisioning.input_verification["cwmp"]["id"] = True
	provisioning.input_verification["cwmp"]["instance_number"] = True
	provisioning.input_verification["cwmp"]["status"] = True

	provisioning.external_msg_template["internal"]["exchange"]["enable"] = True
	provisioning.external_msg_template["internal"]["exchange"]["name"] = provisioning.publisher["internal"]["exchange"]["name"]
	provisioning.external_msg_template["internal"]["exchange"]["type"] = provisioning.publisher["internal"]["exchange"]["type"]
	provisioning.external_msg_template["internal"]["exchange"]["passive"] = False
	provisioning.external_msg_template["internal"]["exchange"]["durable"] = provisioning.publisher["internal"]["exchange"]["durable"]
	provisioning.external_msg_template["internal"]["exchange"]["auto_delete"] = False
	provisioning.external_msg_template["internal"]["exchange"]["internal"] = False
	provisioning.external_msg_template["internal"]["exchange"]["arguments"] = False
	provisioning.external_msg_template["internal"]["queue"]["enable"] = True
	provisioning.external_msg_template["internal"]["queue"]["passive"] = False
	provisioning.external_msg_template["internal"]["queue"]["durable"] = False
	provisioning.external_msg_template["internal"]["queue"]["exclusive"] = False
	provisioning.external_msg_template["internal"]["queue"]["auto_delete"] = False
	provisioning.external_msg_template["internal"]["queue"]["arguments"] = { 'x-expires': 60 * 10 * 1000 }
	provisioning.external_msg_template["internal"]["queue"]["consume"] = False
	provisioning.external_msg_template["internal"]["message"]["enable"] = True
	provisioning.external_msg_template["internal"]["message"]["properties"] = None

	provisioning.internal_msg_template["internal"]["exchange"]["enable"] = False
	provisioning.internal_msg_template["internal"]["exchange"]["name"] = provisioning.consumer["internal"]["exchange"]["name"]
	provisioning.internal_msg_template["internal"]["queue"]["enable"] = False
	provisioning.internal_msg_template["internal"]["message"]["enable"] = True
	provisioning.internal_msg_template["internal"]["message"]["routing_key"] = ""
	provisioning.internal_msg_template["internal"]["message"]["properties"] = None

	database_mongodb = DatabaseMongoDB()
	database_mongodb.config["mongo"]["db"] = "acs"
	database_mongodb.config["mongo"]["collection"] = "wifi_provisioning_1"
	database_mongodb.config["mongo"]["ttl"] = 1200

	database_mongodb.configure()
	database_mongodb.connect()

	provisioning.database_mongodb = database_mongodb

	try:
		provisioning.configure()
		provisioning.run()
	except KeyboardInterrupt:
		provisioning.halt()

if __name__ == '__main__':
	main()

