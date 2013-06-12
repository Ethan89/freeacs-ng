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
import random

# mongo
from bson.objectid import ObjectId

# backend
from provisioning import *
from database_mongodb import *

class WIFIProvisioning(Provisioning):
	def action(self, data):

		try:
			archive = data["archive"]
			for item in archive:
				if (item == self.archive_id_add_object or item == self.archive_id_set_parameter):
					self.LOG.info('already found in history, ignoring message')
					return
		except:
			self.LOG.info('archive not found, creating empty archive')
			archive = [ ]


		if not (data["http"]["username"] == "freecwmp" and data["http"]["password"] == "freecwmp"):
			self.LOG.info('device already provisioned, ignoring message')
			return


		try:
			data["cwmp"]["type"]
		except:
			self.LOG.info('cwmp type not found, ignoring message')
			return


		if data["cwmp"]["type"] == "inform":

			extended_data = copy.deepcopy(data)
			archive.append(self.archive_id_add_object)

		elif data["cwmp"]["type"] == "add_object_response":

			try:
				data["cwmp"]["id"]
				data["cwmp"]["instance_number"]
				data["cwmp"]["status"]
			except:
				self.LOG.error('cwmp message incomplete, aborting request')
				return

			if not data["cwmp"]["status"] == "0":
				self.LOG.error('add object rpc failed, aborting request')
				return

			extended_data = self.database_mongodb.find_one({ "_id": ObjectId(data["cwmp"]["id"]) })

			try:
				archive = extended_data["archive"]
			except:
				self.LOG.error('archive not found in extended data, aborting request')
				return

			archive.append(self.archive_id_set_parameter)

		else:
			self.LOG.info('unhandled cwmp type, ignoring message')
			return


		internal_msg = self.internal_msg_template
		internal_msg["archive"] = archive
		internal_msg["http"] = data["http"]

		try:
			internal_msg["cwmp"] = extended_data["cwmp"]
		except:
			self.LOG.error('cwmp object not found in extended data, aborting request')
			return

		try:
			cpe_oui = extended_data["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.ManufacturerOUI"]
			cpe_sn = extended_data["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.SerialNumber"]
		except:
			self.LOG.error('cwmp parameters not found in extended data, aborting request')
			return

		queue_name = data["http"]["remote_addr"]
		routing_key = "%s.%s" % (cpe_oui, cpe_sn)

		external_msg = copy.deepcopy(self.external_msg_template)

		external_msg["internal"]["queue"]["name"] = queue_name
		external_msg["internal"]["message"]["routing_key"] = routing_key


		if data["cwmp"]["type"] == "inform":

			add_object_id = self.database_mongodb.insert(internal_msg)
			if not add_object_id:
				self.LOG.error('failed to insert data to mongo database, aborting request')
				return

			external_msg["cwmp"]["id"] = "%s" % (add_object_id)
			external_msg["cwmp"]["add_object"] = "InternetGatewayDevice.LANDevice.1.WLANConfiguration."

		elif data["cwmp"]["type"] == "add_object_response":

			external_msg["cwmp"]["id"] = "%016x" % random.getrandbits(64)
			parameter = "InternetGatewayDevice.LANDevice.1.WLANConfiguration.%s.SSID" % (data["cwmp"]["instance_number"])
			external_msg["cwmp"]["set_parameter_values"][parameter] = "OpenWrt"


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

	provisioning.consumer["internal"]["queue"]["name"] = "wifi_provisioning_2"
	provisioning.consumer["internal"]["queue"]["arguments"] = { 'x-expires': 60 * 10 * 1000 }

	provisioning.archive_id_add_object = "%s_%s" % (provisioning.consumer["internal"]["queue"]["name"], "add_object")
	provisioning.archive_id_set_parameter = "%s_%s" % (provisioning.consumer["internal"]["queue"]["name"], "set_parameter_values")

	provisioning.input_verification["http"]["remote_addr"] = True
	provisioning.input_verification["http"]["authorization"]["username"] = True
	provisioning.input_verification["http"]["authorization"]["password"] = True
	provisioning.input_verification["http"]["cookie_level"] = False

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
	database_mongodb.config["mongo"]["collection"] = "wifi_provisioning_2"
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

