#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright 2016-2018 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import mimetypes
import json
import socket

try:
    import uwsgi
except ImportError:
    print >> sys.stderr, "Please run it under uwsgi server"
    exit(-1)

try:
    root = sys.argv[1]
except IndexError:
    print >> sys.stderr, "\nPlease provide a server root as --pyargv command line switch to uwsgi server"
    exit(-1)

def response(errcode, hdrs=None):
    def _(start_response, body='', ct='application/json; charset=UTF-8', ce=None):
        headers = [('Content-Type', ct)]
        if ce:
            headers.append(('Content-Encoding', ce))
        if hdrs:
            headers.extend(hdrs)

        start_response(errcode, headers)
        return [body]

    return _


ok = response("200 Ok")
bad = response("400 Bad request")
notfound = response("404 Not found")
proxy_auth = response("407 Proxy Authentication Required",
                [('Proxy-Authenticate', 'Basic realm=proxytester')]
             )


def content_type_magic(filename):
    mime, encoding = mimetypes.guess_type(filename)

    if not mime:
        mime = 'text/plain'

    return (mime, encoding)


def application(environ, start_response):
    request_method = environ['REQUEST_METHOD']
    path_info = environ['PATH_INFO']
    query_string = environ['QUERY_STRING']
    resource = os.path.join(root, path_info.lstrip('/'))

    def http_headers(environ):
        return '\n'.join(
            '%s=%s' % (k, v) for k, v in environ.iteritems()
            if k.startswith("HTTP_")
        )

    if path_info == "/ws/echo":
        uwsgi.websocket_handshake(environ['HTTP_SEC_WEBSOCKET_KEY'], environ.get('HTTP_ORIGIN', ''))

        msg = uwsgi.websocket_recv()
        uwsgi.websocket_send(json.dumps(
            {
                'headers': {
                    k: str(v) for k, v in environ.iteritems()
                    if k.startswith("HTTP_")
                },
                'message': msg
            },
            indent=True
        ))
        uwsgi.websocket_send(msg)
        return []

    if (path_info.startswith("http://") or path_info.startswith("https://") or request_method == "CONNECT"):
        if query_string == "auth":
            if "HTTP_PROXY_AUTHORIZATION" not in environ:
                return proxy_auth(start_response)

        return ok(start_response,
                  "Proxy request (%s):\n%s" % (request_method, http_headers(environ)),
                  'text/plain')

    if request_method != "GET":
        return bad(start_response, '{"result": "Invalid request"}')

    if path_info == "/echo":
        return ok(start_response,
                  http_headers(environ),
                  'text/plain')

    if path_info == "/__PORT__":
        return ok(start_response, environ["SERVER_PORT"], 'text/plain')

    if path_info == "/__IP__":
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('a.root-servers.net.', 0))
        server_ip, _ = s.getsockname()
        return ok(start_response, server_ip, 'text/plain')

    if os.path.exists(resource):
        if os.path.isfile(resource):
            return ok(start_response,
                      open(resource).read(),
                      *content_type_magic(resource))

        if os.path.isdir(resource):
            return ok(start_response,
                      '\n'.join(
                        '<p><a href="%(fn)s">%(fn)s</a></p>' % {'fn': fn}
                        for fn in os.listdir(resource)
                      ),
                      'text/html')

    return notfound(start_response)
