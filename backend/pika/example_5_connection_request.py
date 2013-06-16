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
import httplib
import urlparse

# backend
from provisioning import *

class ConnectionRequest(Provisioning):
	def action(self, data):

		try:
			ttl = data["internal"]["ttl"] - 1
			if ttl < 0:
				self.LOG.error('ttl exceeded, ignoring message')
				return
		except:
			self.LOG.error('failed to fetch ttl, ignoring message')
			return

		cpe_connection_request_url = data["cwmp"]["parameters"]["InternetGatewayDevice.ManagementServer.ConnectionRequestURL"]
		cpe_connection_request_username = data["cwmp"]["parameters"]["InternetGatewayDevice.ManagementServer.ConnectionRequestUsername"]
		cpe_connection_request_password = data["cwmp"]["parameters"]["InternetGatewayDevice.ManagementServer.ConnectionRequestPassword"]

		http_netloc = urlparse.urlparse(cpe_connection_request_url).netloc
		if not http_netloc:
			self.LOG.error('invalid url, ignoring message')
			return

		http_path = urlparse.urlparse(cpe_connection_request_url).path
		http_auth = base64.encodestring("%s:%s" % (cpe_connection_request_username, cpe_connection_request_password))
		http_headers = { "Authorization" : "Basic %s" % (http_auth) }

		self.LOG.info('initaiting connection request to \"%s\"' % (cpe_connection_request_url))

		failure = False
		try:
			conn = httplib.HTTPConnection(host=http_netloc, timeout=3)
			conn.request("GET", http_path, "", http_headers)

			res = conn.getresponse()

			if not (res.status == 200 or res.status == 204):
				self.LOG.error('received unexpected response status (%d), assuming connection request was completed' % (res.status))

		except httplib.BadStatusLine:
			self.LOG.error('received invalid status line, assuming connection request was completed')

		except:
			self.LOG.error('triggered connection error, re-queuing message')
			failure = True

		else:
			self.LOG.error('completed connection request')

		finally:
			if type(conn) == httplib.HTTPConnection:
				conn.close()


		if failure == True:
			internal_msg_delay = self.internal_msg_delay_template

			internal_msg_delay["archive"] = data["archive"]
			internal_msg_delay["cwmp"] = data["cwmp"]
			internal_msg_delay["http"] = data["http"]

			internal_msg_delay["internal"]["ttl"] = ttl
			internal_msg_delay["internal"]["message"]["properties"]["expiration"] = "%s" % (5 * 1000)

			self.LOG.info('publishing message with content\n%s', json.dumps(internal_msg_delay, separators=(',', ':')))

			self.start_publishing( [ internal_msg_delay ] )

def main():
	provisioning = ConnectionRequest()

	provisioning.config["amqp"]["host"] = "127.0.0.1"
	provisioning.config["amqp"]["virtual_host"] = "/"
	provisioning.config["amqp"]["credentials"]["username"] = "guest"
	provisioning.config["amqp"]["credentials"]["password"] = "guest"
	provisioning.config["amqp"]["connection_attempts"] = 100
	provisioning.config["amqp"]["retry_delay"] = 5

	provisioning.consumer["internal"]["queue"]["name"] = "connection_request_1"
	provisioning.consumer["internal"]["queue"]["arguments"] = { 'x-expires': 60 * 10 * 1000 }

	provisioning.input_verification["http"]["remote_addr"] = True
	provisioning.input_verification["http"]["authorization"]["username"] = True
	provisioning.input_verification["http"]["authorization"]["password"] = True
	provisioning.input_verification["http"]["cookie_level"] = False

	provisioning.input_verification["cwmp"]["type"]["connection_request"] = True
	provisioning.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.ManagementServer.ConnectionRequestURL"] = True
	provisioning.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.ManagementServer.ConnectionRequestUsername"] = True
	provisioning.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.ManagementServer.ConnectionRequestPassword"] = True

	provisioning.internal_msg_delay_template = Dictionary()
	provisioning.internal_msg_delay_template["internal"]["exchange"]["enable"] = True
	provisioning.internal_msg_delay_template["internal"]["exchange"]["name"] = "_internal freeacs-ng (delay exchange)"
	provisioning.internal_msg_delay_template["internal"]["exchange"]["type"] = "direct"
	provisioning.internal_msg_delay_template["internal"]["exchange"]["passive"] = False
	provisioning.internal_msg_delay_template["internal"]["exchange"]["durable"] = True
	provisioning.internal_msg_delay_template["internal"]["exchange"]["auto_delete"] = True
	provisioning.internal_msg_delay_template["internal"]["exchange"]["internal"] = False
	provisioning.internal_msg_delay_template["internal"]["queue"]["enable"] = True
	provisioning.internal_msg_delay_template["internal"]["queue"]["name"] = "_internal freeacs-ng (delay queue)"
	provisioning.internal_msg_delay_template["internal"]["queue"]["passive"] = False
	provisioning.internal_msg_delay_template["internal"]["queue"]["durable"] = False
	provisioning.internal_msg_delay_template["internal"]["queue"]["exclusive"] = False
	provisioning.internal_msg_delay_template["internal"]["queue"]["auto_delete"] = False
	provisioning.internal_msg_delay_template["internal"]["queue"]["arguments"] = {
			'x-expires' : 3600 * 1000,
			'x-message-ttl' : 900 * 1000,
			'x-dead-letter-exchange' : provisioning.consumer["internal"]["exchange"]["name"],
			'x-dead-letter-routing-key' : ""
		}
	provisioning.internal_msg_delay_template["internal"]["queue"]["consume"] = False
	provisioning.internal_msg_delay_template["internal"]["message"]["enable"] = True
	provisioning.internal_msg_delay_template["internal"]["message"]["routing_key"] = ""
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["content_type"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["content_encoding"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["headers"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["delivery_mode"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["priority"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["correlation_id"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["reply_to"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["expiration"] = "%s" % (900 * 1000)
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["message_id"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["timestamp"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["type"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["user_id"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["app_id"] = None
	provisioning.internal_msg_delay_template["internal"]["message"]["properties"]["cluster_id"] = None

	try:
		provisioning.configure()
		provisioning.run()
	except KeyboardInterrupt:
		provisioning.halt()

if __name__ == '__main__':
	main()

