#!/usr/bin/env python3
# Copyright (c) 2015-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multiple RPC users."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_datadir_path,
    str_to_b64str,
)

import os
import http.client
import urllib.parse
import subprocess
from random import SystemRandom
import string
import configparser
import sys


def call_with_auth(node, user, password):
    url = urllib.parse.urlparse(node.url)
    headers = {"Authorization": "Basic " + str_to_b64str('{}:{}'.format(user, password))}

    conn = http.client.HTTPConnection(url.hostname, url.port)
    conn.connect()
    conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
    resp = conn.getresponse()
    conn.close()
    return resp


class HTTPBasicsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.supports_cli = False

    def setup_chain(self):
        super().setup_chain()
        #Append rpcauth to bitcoin.conf before initialization
        rpcauth = "rpcauth=rt:93648e835a54c573682c2eb19f882535$7681e9c5b74bdd85e78166031d2058e1069b3ed7ed967c93fc63abba06f31144"
        rpcauth2 = "rpcauth=rt2:f8607b1a88861fac29dfccf9b52ff9f$ff36a0c23c8c62b4846112e50fa888416e94c17bfd4c42f88fd8f55ec6a3137e"
        rpcuser = "rpcuser=rpcuserïýyŽ¹"
        rpcpassword = "rpcpassword=rpcpasswordïýz½ƒ

        self.user = ''.join(SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(10))
        config = configparser.ConfigParser()
        config.read_file(open(self.options.configfile))
        gen_rpcauth = config['environment']['RPCAUTH']
        p = subprocess.Popen([sys.executable, gen_rpcauth, self.user], stdout=subprocess.PIPE, universal_newlines=True)
        lines = p.stdout.read().splitlines()
        rpcauth3 = lines[1]
        self.password = lines[3]

        with open(os.path.join(get_datadir_path(self.options.tmpdir, 0), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write(rpcauth + "\n")
            f.write(rpcauth2 + "\n")
            f.write(rpcauth3 + "\n")
        with open(os.path.join(get_datadir_path(self.options.tmpdir, 1), "bitcoin.conf"), 'a', encoding='utf8') as f:
            f.write("rpcuser={}\n".format(self.rpcuser))
            f.write("rpcpassword={}\n".format(self.rpcpassword))

        self.log.info('Correct...')
        headers = {"Authorization": "Basic " + str_to_b64str(authpair)}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        resp = conn.getresponse()
        assert_equal(resp.status, 200)
        conn.close()

        #Use new authpair to confirm both work
        self.log.info('Correct...')
        headers = {"Authorization": "Basic " + str_to_b64str(authpairnew)}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        resp = conn.getresponse()
        assert_equal(resp.status, 200)
        conn.close()

        #Wrong login name with rt's password
        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(node, user, password + 'wrong').status)

        #Wrong password for rt
        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(node, user + 'wrong', password).status)

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        resp = conn.getresponse()
        assert_equal(resp.status, 401)
        conn.close()

        #Correct for rt2
        self.log.info('Correct...')
        authpairnew = "rt2:"+password2
        headers = {"Authorization": "Basic " + str_to_b64str(authpairnew)}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        resp = conn.getresponse()
        assert_equal(resp.status, 200)
        conn.close()

        #Wrong password for rt2
        self.log.info('Wrong...')
        assert_equal(401, call_with_auth(node, user + 'wrong', password + 'wrong').status)

    def run_test(self):
        self.log.info('Check correctness of the rpcauth config option')
        url = urllib.parse.urlparse(self.nodes[0].url)

        #Wrong password for randomly generated user
        self.log.info('Wrong...')
        authpairnew = self.user+":"+self.password+"Wrong"
        headers = {"Authorization": "Basic " + str_to_b64str(authpairnew)}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        resp = conn.getresponse()
        assert_equal(resp.status, 401)
        conn.close()

        self.log.info('Check correctness of the rpcuser/rpcpassword config options')
        url = urllib.parse.urlparse(self.nodes[1].url)

        # rpcuser and rpcpassword authpair
        self.log.info('Correct...')
        rpcuserauthpair = "rpcuserïýyŽ¹:rpcpasswordïýz½ƒ

        headers = {"Authorization": "Basic " + str_to_b64str(rpcuserauthpair)}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        resp = conn.getresponse()
        assert_equal(resp.status, 200)
        conn.close()

        #Wrong login name with rpcuser's password
        rpcuserauthpair = "rpcuserwrong:rpcpassword"
        headers = {"Authorization": "Basic " + str_to_b64str(rpcuserauthpair)}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        resp = conn.getresponse()
        assert_equal(resp.status, 401)
        conn.close()

        #Wrong password for rpcuser
        self.log.info('Wrong...')
        rpcuserauthpair = "rpcuser:rpcpasswordwrong"
        headers = {"Authorization": "Basic " + str_to_b64str(rpcuserauthpair)}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        resp = conn.getresponse()
        assert_equal(resp.status, 401)
        conn.close()


        self.log.info('Check that failure to write cookie file will abort the node gracefully')
        self.stop_node(0)
        cookie_file = os.path.join(get_datadir_path(self.options.tmpdir, 0), self.chain, '.cookie.tmp')
        os.mkdir(cookie_file)
        init_error = 'Error: Unable to start HTTP server. See debug log for details.'
        self.nodes[0].assert_start_raises_init_error(expected_msg=init_error)


if __name__ == '__main__':
    HTTPBasicsTest().main()
