#!/usr/bin/env python2
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# Copyright (C) 2013 Luka Perkov <freeacs-ng@lukaperkov.net>
#

import random

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
	time_provisioning = TimeProvisioning()

	time_provisioning.config["amqp"]["host"] = "127.0.0.1"
	time_provisioning.config["amqp"]["virtual_host"] = "/"
	time_provisioning.config["amqp"]["credentials"]["username"] = "guest"
	time_provisioning.config["amqp"]["credentials"]["password"] = "guest"
	time_provisioning.config["amqp"]["connection_attempts"] = 100
	time_provisioning.config["amqp"]["retry_delay"] = 5

	time_provisioning.consumer["internal"]["queue"]["name"] = "time_provisioning_1"
	time_provisioning.consumer["internal"]["queue"]["arguments"] = { 'x-expires': 60 * 10 * 1000 }

	time_provisioning.input_verification["http"]["remote_addr"] = True
	time_provisioning.input_verification["http"]["authorization"]["username"] = True
	time_provisioning.input_verification["http"]["authorization"]["password"] = True
	time_provisioning.input_verification["http"]["cookie_level"] = False

	time_provisioning.input_verification["cwmp"]["type"]["inform"] = True
	time_provisioning.input_verification["cwmp"]["events"]["bootstrap"] = False
	time_provisioning.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.ManufacturerOUI"] = True
	time_provisioning.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.SerialNumber"] = True

	time_provisioning.external_msg_template["internal"]["exchange"]["enable"] = True
	time_provisioning.external_msg_template["internal"]["exchange"]["name"] = time_provisioning.publisher["internal"]["exchange"]["name"]
	time_provisioning.external_msg_template["internal"]["exchange"]["type"] = time_provisioning.publisher["internal"]["exchange"]["type"]
	time_provisioning.external_msg_template["internal"]["exchange"]["passive"] = False
	time_provisioning.external_msg_template["internal"]["exchange"]["durable"] = time_provisioning.publisher["internal"]["exchange"]["durable"]
	time_provisioning.external_msg_template["internal"]["exchange"]["auto_delete"] = False
	time_provisioning.external_msg_template["internal"]["exchange"]["internal"] = False
	time_provisioning.external_msg_template["internal"]["exchange"]["arguments"] = False
	time_provisioning.external_msg_template["internal"]["queue"]["enable"] = True
	time_provisioning.external_msg_template["internal"]["queue"]["passive"] = False
	time_provisioning.external_msg_template["internal"]["queue"]["durable"] = False
	time_provisioning.external_msg_template["internal"]["queue"]["exclusive"] = False
	time_provisioning.external_msg_template["internal"]["queue"]["auto_delete"] = False
	time_provisioning.external_msg_template["internal"]["queue"]["arguments"] = { 'x-expires': 60 * 10 * 1000 }
	time_provisioning.external_msg_template["internal"]["queue"]["consume"] = False
	time_provisioning.external_msg_template["internal"]["message"]["enable"] = True
	time_provisioning.external_msg_template["internal"]["message"]["properties"] = None

	time_provisioning.internal_msg_template["internal"]["exchange"]["enable"] = False
	time_provisioning.internal_msg_template["internal"]["exchange"]["name"] = time_provisioning.consumer["internal"]["exchange"]["name"]
	time_provisioning.internal_msg_template["internal"]["queue"]["enable"] = False
	time_provisioning.internal_msg_template["internal"]["message"]["enable"] = True
	time_provisioning.internal_msg_template["internal"]["message"]["routing_key"] = ""
	time_provisioning.internal_msg_template["internal"]["message"]["properties"] = None

	try:
		time_provisioning.configure()
		time_provisioning.run()
	except KeyboardInterrupt:
		time_provisioning.halt()

if __name__ == '__main__':
	main()

