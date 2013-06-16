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
import random

# backend
from provisioning import *

class TimeProvisioning(Provisioning):
	def action(self, data):

		internal_msg = self.internal_msg_template

		try:
			archive = data["archive"]
			for item in archive:
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

		external_msg["cwmp"]["id"] = "%016x" % random.getrandbits(64)
		external_msg["cwmp"]["set_parameter_values"]["InternetGatewayDevice.ManagementServer.PeriodicInformEnable"] = 1
		external_msg["cwmp"]["set_parameter_values"]["InternetGatewayDevice.ManagementServer.PeriodicInformInterval"] = 60
		self.LOG.info('publishing message with content\n%s', json.dumps(external_msg, separators=(',', ':')))
		self.LOG.info('publishing message with content\n%s', json.dumps(internal_msg, separators=(',', ':')))

		self.start_publishing( [ external_msg, internal_msg ] )


def main():
	provisioning = TimeProvisioning()

	provisioning.config["amqp"]["host"] = "127.0.0.1"
	provisioning.config["amqp"]["virtual_host"] = "/"
	provisioning.config["amqp"]["credentials"]["username"] = "guest"
	provisioning.config["amqp"]["credentials"]["password"] = "guest"
	provisioning.config["amqp"]["connection_attempts"] = 100
	provisioning.config["amqp"]["retry_delay"] = 5

	provisioning.consumer["internal"]["queue"]["name"] = "time_provisioning_1"
	provisioning.consumer["internal"]["queue"]["arguments"] = { 'x-expires': 60 * 10 * 1000 }

	provisioning.input_verification["http"]["remote_addr"] = True
	provisioning.input_verification["http"]["authorization"]["username"] = True
	provisioning.input_verification["http"]["authorization"]["password"] = True
	provisioning.input_verification["http"]["cookie_level"] = False

	provisioning.input_verification["cwmp"]["type"]["inform"] = True
	provisioning.input_verification["cwmp"]["events"]["bootstrap"] = False
	provisioning.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.ManufacturerOUI"] = True
	provisioning.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.SerialNumber"] = True

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

	try:
		provisioning.configure()
		provisioning.run()
	except KeyboardInterrupt:
		provisioning.halt()

if __name__ == '__main__':
	main()

