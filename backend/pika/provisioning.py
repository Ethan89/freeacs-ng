#!/usr/bin/env python2
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# Copyright (C) 2013 Luka Perkov <freeacs-ng@lukaperkov.net>
#

import base64
import json
import logging
import pika
import sys
import time
import re

from functools import partial

class Dictionary(dict):

	def __getitem__(self, item):

		try:
			return dict.__getitem__(self, item)

		except KeyError:
			value = self[item] = type(self)()
			return value


class Provisioning(object):
	LOG = logging.getLogger(__name__)

	def __init__(self):
		self._credentials = None
		self._parameters = None
		self._connection = None
		self._channel = None
		self._consumer_tag = None

		self.external_msg_template = Dictionary()
		self.internal_msg_template = Dictionary()
		self.input_verification = Dictionary()

		self.config = Dictionary()
		self.config["amqp"]["host"] = None
		self.config["amqp"]["port"] = None
		self.config["amqp"]["virtual_host"] = None
		self.config["amqp"]["credentials"]["username"] = None
		self.config["amqp"]["credentials"]["password"] = None
		self.config["amqp"]["channel_max"] = None
		self.config["amqp"]["frame_max"] = None
		self.config["amqp"]["heartbeat_interval"] = None
		self.config["amqp"]["connection_attempts"] = None
		self.config["amqp"]["retry_delay"] = None
		self.config["amqp"]["socket_timeout"] = None
		self.config["amqp"]["ssl"] = None
		self.config["amqp"]["ssl_options"] = None
		self.config["amqp"]["locale"] = None
		self.config["amqp"]["backpressure_detection"] = None
		self.config["config"]["regex_ipv4_address"] = "^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$"
		self.config["config"]["regex_cpe_oui"] = "^[0-9A-Fa-f]{6}$"
		self.config["config"]["regex_cpe_sn"] = "^[a-zA-Z0-9]{12,64}$"
		self.config["config"]["regex_cpe_software_version"] = "^[\w\d\s-]{0,32}$"
		self.config["config"]["log"]["level"] = logging.INFO
		self.config["config"]["log"]["format"] = "%(asctime)s  %(funcName) -40s: %(message)s"

		self.consumer = Dictionary()
		self.consumer["internal"]["exchange"]["enable"] = True
		self.consumer["internal"]["exchange"]["name"] = 'freeacs-ng broadcast fanout'
		self.consumer["internal"]["exchange"]["type"] = 'fanout'
		self.consumer["internal"]["exchange"]["passive"] = False
		self.consumer["internal"]["exchange"]["durable"] = True
		self.consumer["internal"]["exchange"]["auto_delete"] = False
		self.consumer["internal"]["exchange"]["internal"] = False
		self.consumer["internal"]["exchange"]["arguments"] = None
		self.consumer["internal"]["queue"]["enable"] = True
		self.consumer["internal"]["queue"]["name"] = 'freeacs-ng broadcast fanout'
		self.consumer["internal"]["queue"]["passive"] = False
		self.consumer["internal"]["queue"]["durable"] = True
		self.consumer["internal"]["queue"]["exclusive"] = False
		self.consumer["internal"]["queue"]["auto_delete"] = False
		self.consumer["internal"]["queue"]["arguments"] = None
		self.consumer["internal"]["queue"]["consume"] = True
		self.consumer["internal"]["message"]["enable"] = False
		self.consumer["internal"]["message"]["routing_key"] = None

		self.publisher = Dictionary()
		self.publisher["internal"]["exchange"]["enable"] = True
		self.publisher["internal"]["exchange"]["name"] = 'freeacs-ng provisioning topic'
		self.publisher["internal"]["exchange"]["type"] = 'topic'
		self.publisher["internal"]["exchange"]["passive"] =False
		self.publisher["internal"]["exchange"]["durable"] = True
		self.publisher["internal"]["exchange"]["auto_delete"] = False
		self.publisher["internal"]["exchange"]["internal"] = False
		self.publisher["internal"]["exchange"]["arguments"] = None
		self.publisher["internal"]["queue"]["enable"] = False
		self.publisher["internal"]["message"]["enable"] = False

	def configure(self):
		self._regex_ipv4_address = re.compile(self.config["config"]["regex_ipv4_address"])
		self._regex_cpe_oui = re.compile(self.config["config"]["regex_cpe_oui"])
		self._regex_cpe_sn = re.compile(self.config["config"]["regex_cpe_sn"])
		self._regex_cpe_software_version = re.compile(self.config["config"]["regex_cpe_software_version"])

		logging.basicConfig(level=self.config["config"]["log"]["level"],
				    format=self.config["config"]["log"]["format"])

		self.LOG.info('freeacs-ng python pika backend initialized')

	def connection_open(self):
		self.LOG.info('opening connection')

		self._credentials = pika.PlainCredentials(username=self.config["amqp"]["credentials"]["username"],
							  password=self.config["amqp"]["credentials"]["password"],
							  erase_on_connect=True)

		self._parameters = pika.ConnectionParameters(host=self.config["amqp"]["host"],
							     port=self.config["amqp"]["port"],
							     virtual_host=self.config["amqp"]["virtual_host"],
							     credentials=self._credentials,
							     channel_max=self.config["amqp"]["channel_max"],
							     frame_max=self.config["amqp"]["frame_max"],
							     heartbeat_interval=self.config["amqp"]["heartbeat_interval"],
							     ssl=self.config["amqp"]["ssl"],
							     ssl_options=self.config["amqp"]["ssl_options"],
							     connection_attempts=self.config["amqp"]["connection_attempts"],
							     retry_delay=self.config["amqp"]["retry_delay"],
							     socket_timeout=self.config["amqp"]["socket_timeout"],
							     locale=self.config["amqp"]["locale"],
							     backpressure_detection=self.config["amqp"]["backpressure_detection"])

		self._connection = pika.SelectConnection(parameters=self._parameters,
							 on_open_callback=self.on_connection_open,
							 on_open_error_callback=self.on_connection_open_error,
							 stop_ioloop_on_close=False)

	def on_connection_open(self, unused_connection):
		self.LOG.info('connection opened')
		self.add_on_connection_close_callback()
		self.channel_open()

	def on_connection_open_error(self, unused_connection):
		self.LOG.warning('connection open error')
		sys.exit(1)

	def connection_close(self):
		if not self._connection:
			return

		self.LOG.info('closing connection')
		self._connection.close()

	def on_connection_closed(self, connection, reply_code, reply_text):
		self.LOG.warning('connection closed, reopening in %s seconds: (%s) %s',
				 self.config["amqp"]["retry_delay"], reply_code, reply_text)

		self._channel = None
		self._connection.add_timeout(self.config["amqp"]["retry_delay"], self.reconnect)

	def add_on_connection_close_callback(self):
		self.LOG.info('adding connection close callback')
		self._connection.add_on_close_callback(self.on_connection_closed)

	def reconnect(self):
		self.LOG.info('reconnecting to rabbitmq')
		self._connection.ioloop.stop()
		self.connection_open()
		self._connection.ioloop.start()

	def channel_open(self):
		self.LOG.info('creating channel')
		self._connection.channel(on_open_callback=self.on_channel_open)

	def on_channel_open(self, channel):
		self.LOG.info('channel opened')
		self._channel = channel
		self.add_on_channel_close_callback()
		self.add_on_channel_cancel_callback()
		self.add_on_delivery_confirmation_callback()

		self.setup_exchange([ self.consumer ])
		self.setup_exchange([ self.publisher ])

	def channel_close(self):
		if not self._channel:
			return

		self.LOG.info('closing channel')
		self._channel.close()

	def on_channel_closed(self, channel, reply_code, reply_text):
		self.LOG.warning('channel closed: (%s) %s', reply_code, reply_text)
		self._connection.close()

	def add_on_channel_close_callback(self):
		self._channel.add_on_close_callback(self.on_channel_closed)

	def on_channel_cancelled(self, method_frame):
		self.LOG.info('cancelled remotely, shutting down')
		self._channel.close()

	def add_on_channel_cancel_callback(self):
		self._channel.add_on_cancel_callback(self.on_channel_cancelled)

	def on_delivery_confirmation(self, method_frame):
		confirmation_type = method_frame.method.NAME.split('.')[1].lower()
		self.LOG.info('received (%s) for delivery tag (%i)',
			      confirmation_type,
			      method_frame.method.delivery_tag)

	def add_on_delivery_confirmation_callback(self):
		self.LOG.info('issuing confirm.select rpc command')
		self._channel.confirm_delivery(self.on_delivery_confirmation)

	def setup_exchange(self, messages):
		if not messages[0]["internal"]["exchange"]["enable"] == True:
			self.LOG.info('skipping exchange setup')
			self.setup_queue(messages)
			return

		self.LOG.info('declaring exchange "%s"', messages[0]["internal"]["exchange"]["name"])

		callback = partial(self.on_exchange_declare_ok, messages)
		self._channel.exchange_declare(callback,
					       exchange=messages[0]["internal"]["exchange"]["name"],
					       exchange_type=messages[0]["internal"]["exchange"]["type"],
					       passive=messages[0]["internal"]["exchange"]["passive"],
					       durable=messages[0]["internal"]["exchange"]["durable"],
					       auto_delete=messages[0]["internal"]["exchange"]["auto_delete"],
					       internal=messages[0]["internal"]["exchange"]["internal"],
					       arguments=messages[0]["internal"]["exchange"]["arguments"])

	def on_exchange_declare_ok(self, messages, unused_frame):
		self.LOG.info('declared exchange "%s"' % messages[0]["internal"]["exchange"]["name"])

		self.setup_queue(messages)

	def setup_queue(self, messages):

		if not messages[0]["internal"]["queue"]["enable"] == True:
			self.LOG.info('skipping queue setup')
			self.setup_bind(messages)
			return

		self.LOG.info('declaring queue "%s"', messages[0]["internal"]["queue"]["name"])

		callback = partial(self.on_queue_declare_ok, messages)
		self._channel.queue_declare(callback,
					    queue=messages[0]["internal"]["queue"]["name"],
					    passive=messages[0]["internal"]["queue"]["passive"],
					    durable=messages[0]["internal"]["queue"]["durable"],
					    exclusive=messages[0]["internal"]["queue"]["exclusive"],
					    auto_delete=messages[0]["internal"]["queue"]["auto_delete"],
					    arguments=messages[0]["internal"]["queue"]["arguments"])

	def on_queue_declare_ok(self, messages, method_frame):
		self.LOG.info('declared queue "%s"' % messages[0]["internal"]["queue"]["name"])

		self.setup_bind(messages)

	def setup_bind(self, messages):
		if not messages[0]["internal"]["exchange"]["enable"] == True:
			self.LOG.info('skipping bind setup')
			self.publish(messages)
			return

		if not messages[0]["internal"]["queue"]["enable"] == True:
			self.LOG.info('skipping bind setup')
			self.publish(messages)
			return

		self.LOG.info('binding queue "%s" to exchange "%s"' % (messages[0]["internal"]["queue"]["name"], messages[0]["internal"]["exchange"]["name"]))

		callback = partial(self.on_bind_ok, messages)
		self._channel.queue_bind(callback,
					 exchange=messages[0]["internal"]["exchange"]["name"],
					 queue=messages[0]["internal"]["queue"]["name"],
					 routing_key=messages[0]["internal"]["message"]["routing_key"])

	def on_bind_ok(self, messages, unused_frame):
		self.LOG.info('bound queue "%s" to exchange "%s"' % (messages[0]["internal"]["queue"]["name"], messages[0]["internal"]["exchange"]["name"]))

		if messages[0]["internal"]["queue"]["consume"]:
			self.consume()
			return

		self.publish(messages)

	def publish(self, messages):
		if not messages[0]["internal"]["message"]["enable"] == True:
			self.LOG.info('skipping message publishing')
			self.handle(messages)
			return

		self.LOG.info('publishing message')
		self._channel.basic_publish(exchange=messages[0]["internal"]["exchange"]["name"],
					    routing_key=messages[0]["internal"]["message"]["routing_key"],
					    body=json.dumps(messages[0], separators=(',', ':')),
					    properties=messages[0]["internal"]["message"]["properties"])

		self.handle(messages)

	def handle(self, messages):
		try:
			messages.pop(0)
		except:
			self.LOG.info('no more messages in the list')
			return

		if not messages:
			self.LOG.info('no more messages in the list')
			return

		self.start_publishing(messages)

	def consume(self):
		self.LOG.info('consuming messages')
		self._consumer_tag = \
			self._channel.basic_consume(self.on_message,
						    self.consumer["internal"]["queue"]["name"])

	def stop_consuming(self):
		if not self._consumer_tag:
			return

		self.LOG.info('stopping consuming')
		self._channel.basic_cancel(self.on_cancel_ok, self._consumer_tag)

	def on_cancel_ok(self, unused_frame):
		self.LOG.info('consuming stopped')
		self.connection_close()

	def acknowledge_message(self, delivery_tag):
		self.LOG.info('acknowledging message with tag (%s)', delivery_tag)
		self._channel.basic_ack(delivery_tag)

	def on_message(self, unused_channel, basic_deliver, properties, body):
		self.LOG.info('received message tag (%s) with content\n%s',
			      basic_deliver.delivery_tag, body)

		self.acknowledge_message(basic_deliver.delivery_tag)

		try:
			data = json.loads(body)
		except:
			self.LOG.error('failed to load json body')
			return

		if self.input_verification["http"]["remote_addr"] == True:
			try:
				remote_addr = data["http"]["remote_addr"]
				if not self._regex_ipv4_address.match(remote_addr):
					self.LOG.error('http remote_addr "%s" did not match regular expression' % (remote_addr))
					return
			except:
				self.LOG.error('failed to fetch http remote_addr')
				return

		if self.input_verification["http"]["authorization"]["username"] == True:
			try:
				encoded_authorization = data["http"]["authorization"].split()[1]
				http_username = base64.b64decode(encoded_authorization).split(':')[0]
				data["http"]["username"] = http_username
			except:
				self.LOG.error('failed to fetch http authorization username')
				return

		if self.input_verification["http"]["authorization"]["password"] == True:
			try:
				encoded_authorization = data["http"]["authorization"].split()[1]
				http_password = base64.b64decode(encoded_authorization).split(':')[1]
				data["http"]["password"] = http_password
			except:
				self.LOG.error('failed to fetch http authorization password')
				return

		if self.input_verification["http"]["cookie_level"] == True:
			http_cookie_level = None
			try:
				cookies = data["http"]["cookie"].replace(';', ' ')
				for cookie in cookies.split():
					key = cookie.split('=')[0]
					val = cookie.split('=')[1]
					if key.lower() == "level":
						http_cookie_level = val.lower()
			except:
				self.LOG.info('failed to fetch http cookie level')

			if not http_cookie_level:
				self.LOG.info('http cookie level was not found')
				return

			data["http"]["cookie_level"] = http_cookie_level

		if self.input_verification["cwmp"]["type"]["inform"] == True:
			try:
				message_type = data["cwmp"]["type"]
				if not message_type == "inform":
					LOG.info('ignoring non inform message')
					return
			except:
				self.LOG.error('failed to fetch cwmp type')
				return

		if self.input_verification["cwmp"]["type"]["add_object_response"] == True:
			try:
				message_type = data["cwmp"]["type"]
				if not message_type == "add_object_response":
					LOG.info('ignoring non add_object_response message')
					return
			except:
				self.LOG.error('failed to fetch cwmp type')
				return

		if self.input_verification["cwmp"]["id"] == True:
			try:
				data["cwmp"]["id"]
			except:
				self.LOG.error('failed to fetch cwmp id"')
				return

		if self.input_verification["cwmp"]["instance_number"] == True:
			try:
				data["cwmp"]["instance_number"]
			except:
				self.LOG.error('failed to fetch cwmp instance_number"')
				return

		if self.input_verification["cwmp"]["status"] == True:
			try:
				data["cwmp"]["status"]
			except:
				self.LOG.error('failed to fetch cwmp status"')
				return

		if self.input_verification["cwmp"]["events"]["bootstrap"] == True:
			try:
				events = data["cwmp"]["events"]
				if not "bootstrap" in events:
					self.LOG.info('no bootstrap event thus ignoring the message')
					return
			except:
				self.LOG.error('failed to fetch cwmp events')
				return

		if self.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.ManufacturerOUI"] == True:
			try:
				cpe_oui = data["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.ManufacturerOUI"]
				if not self._regex_cpe_oui.match(cpe_oui):
					self.LOG.error('"InternetGatewayDevice.DeviceInfo.ManufacturerOUI" "%s" did not match regular expression' % (cpe_oui))
					return
			except:
				self.LOG.error('failed to fetch "InternetGatewayDevice.DeviceInfo.ManufacturerOUI"')
				return

		if self.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.SerialNumber"] == True:
			try:
				cpe_sn = data["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.SerialNumber"]
				if not self._regex_cpe_sn.match(cpe_sn):
					self.LOG.error('"InternetGatewayDevice.DeviceInfo.SerialNumber" "%s" did not match regular expression' % (cpe_sn))
					return
			except:
				self.LOG.error('failed to fetch "InternetGatewayDevice.DeviceInfo.SerialNumber"')
				return

		if self.input_verification["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.SoftwareVersion"] == True:
			try:
				cpe_software_version = data["cwmp"]["parameters"]["InternetGatewayDevice.DeviceInfo.SoftwareVersion"]
				if not self._regex_cpe_software_version.match(cpe_software_version):
					self.LOG.error('"InternetGatewayDevice.DeviceInfo.SoftwareVersion" "%s" did not match regular expression' % (cpe_software_version))
					return
			except:
				self.LOG.error('failed to fetch cpe_software_version')
				return

		self.action(data)

	def action(self, messages):
		pass

	def start_publishing(self, messages):
		self.setup_exchange(messages)

	def run(self):
		self.connection_open()
		self._connection.ioloop.start()

	def halt(self):
		self.LOG.info('stopping')
		self.stop_consuming()
		self.channel_close()
		self.connection_close()

