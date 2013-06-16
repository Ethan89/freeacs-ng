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

		internal_msg = self.internal_msg_template

		try:
			archive = data["archive"]
			for item in archive:
				if item == "time_provisioning_1":
					self.LOG.info('running in separate thread, ignoring message')
					return

				if item == self.consumer["internal"]["queue"]["name"]:
					self.LOG.info('already found in history, ignoring message')
					return
		except:
			self.LOG.info('archive not found, creating empty archive')
			archive = [ ]

		archive.append(self.consumer["internal"]["queue"]["name"])
		internal_msg["archive"] = archive

		if not (data["http"]["username"] == "freecwmp" and data["http"]["password"] == "freecwmp"):
			self.LOG.info('device already provisioned, ignoring message')
			return


		internal_msg["cwmp"] = data["cwmp"]
		internal_msg["http"] = data["http"]


		cpe_oui = data["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.ManufacturerOUI"]
		cpe_sn = data["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.SerialNumber"]

		queue_name = data["http"]["remote_addr"]
		routing_key = "%s.%s" % (cpe_oui, cpe_sn)

		external_msg = self.external_msg_template

		external_msg["internal"]["queue"]["name"] = queue_name
		external_msg["internal"]["message"]["routing_key"] = routing_key

		mongo_msg = Dictionary()
		mongo_msg["archive"] = internal_msg["archive"]
		mongo_msg["cwmp"] = data["cwmp"]
		mongo_msg["http"] = data["http"]

		mongo_external_msg = copy.deepcopy(external_msg)
		mongo_external_msg["cwmp"]["set_parameter_values"]["InternetGatewayDevice.LANDevice.1.WLANConfiguration.{i}.SSID"] = "OpenWrt"
		mongo_external_msg_enc = base64.b64encode('%s' % (json.dumps(mongo_external_msg, separators=(',', ':'))))

		mongo_internal_msg_1 = copy.deepcopy(internal_msg)
		mongo_msg["archive"].append("wifi_provisioning_1_set_parameter_values")
		mongo_internal_msg_enc_1 = base64.b64encode('%s' % (json.dumps(mongo_internal_msg_1, separators=(',', ':'))))

		mongo_internal_msg_2 = copy.deepcopy(internal_msg)
		mongo_internal_msg_2["cwmp"]["type"] = "connection_request"
		mongo_internal_msg_2["cwmp"]["parameters"]["InternetGatewayDevice.ManagementServer.ConnectionRequestUsername"] = "freeacs-ng"
		mongo_internal_msg_2["cwmp"]["parameters"]["InternetGatewayDevice.ManagementServer.ConnectionRequestPassword"] = "freeacs-ng"
		mongo_internal_msg_2["internal"]["ttl"] = 5
		mongo_internal_msg_enc_2 = base64.b64encode('%s' % (json.dumps(mongo_internal_msg_2, separators=(',', ':'))))

		mongo_msg["relay"]["id"] = self.consumer["internal"]["queue"]["name"]
		mongo_msg["relay"]["structure"] = "base64"
		mongo_msg["relay"]["amqp"] = [ mongo_external_msg_enc, mongo_internal_msg_enc_1, mongo_internal_msg_enc_2 ]

		add_object_id = self.database_mongodb.insert(mongo_msg)
		if not add_object_id:
			self.LOG.error('failed to insert data to mongo database, aborting request')
			return

		external_msg["cwmp"]["id"] = "%s" % (add_object_id)
		external_msg["cwmp"]["add_object"] = "InternetGatewayDevice.LANDevice.1.WLANConfiguration."

		self.LOG.info('publishing message with content\n%s', json.dumps(external_msg, separators=(',', ':')))
		self.LOG.info('publishing message with content\n%s', json.dumps(internal_msg, separators=(',', ':')))

		self.start_publishing( [ external_msg, internal_msg ] )


def main():
	provisioning = WIFIProvisioning()

	provisioning.config["amqp"]["host"] = "127.0.0.1"
	provisioning.config["amqp"]["virtual_host"] = "/"
	provisioning.config["amqp"]["credentials"]["username"] = "guest"
	provisioning.config["amqp"]["credentials"]["password"] = "guest"
	provisioning.config["amqp"]["connection_attempts"] = 100
	provisioning.config["amqp"]["retry_delay"] = 5

	provisioning.consumer["internal"]["queue"]["name"] = "wifi_provisioning_1_add_object"
	provisioning.consumer["internal"]["queue"]["arguments"] = { 'x-expires': 60 * 10 * 1000 }

	provisioning.input_verification["http"]["remote_addr"] = True
	provisioning.input_verification["http"]["authorization"]["username"] = True
	provisioning.input_verification["http"]["authorization"]["password"] = True
	provisioning.input_verification["http"]["cookie_level"] = False

	provisioning.input_verification["cwmp"]["type"]["inform"] = True
	provisioning.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.ManufacturerOUI"] = True
	provisioning.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.SerialNumber"] = True
	provisioning.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.ManagementServer.ConnectionRequestURL"] = True

	provisioning.external_msg_template = Dictionary()
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

	provisioning.internal_msg_template = Dictionary()
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

