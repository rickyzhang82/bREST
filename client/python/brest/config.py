from __future__ import absolute_import, division, print_function, unicode_literals
import json
import os
import sys

if 2 == sys.version_info[0]:
    text = unicode
else:
    text = str


class Config(object):
    CFG_KEY_SERVER_PORT = "server.port"
    CFG_KEY_SERVER_URL = "server.url"
    CFG_KEY_SYSTEM_DEBUG_ON = "system.debug_on"

    CONFIG_FILE_NAME = "config.json"

    WRITABLE_KEY_LIST = (CFG_KEY_SERVER_PORT, CFG_KEY_SERVER_URL,
                         CFG_KEY_SYSTEM_DEBUG_ON)

    READABLE_KEY_LIST = (CFG_KEY_SERVER_PORT, CFG_KEY_SERVER_URL,
                         CFG_KEY_SYSTEM_DEBUG_ON)

    SAVE_KEY_LIST = (CFG_KEY_SERVER_PORT, CFG_KEY_SERVER_URL,
                     CFG_KEY_SYSTEM_DEBUG_ON)

    def __init__(self, load_default_json_config_file=True, config_file_loc=None, auto_save=False):
        """
        Config constructor

        Args:
            load_default_json_config_file: boolean flag to indicate if load default config from JSON file
            config_file_loc: config file path
            auto_save: auto save to config file

        """
        self._auto_save = False
        if load_default_json_config_file:
            self._conf = None
            self._config_file_path = None
            relative_conf_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), "conf")
            for loc in config_file_loc, relative_conf_path, os.path.expanduser("~"), "/etc", os.environ.get("HOME"):
                try:
                    if loc is not None:
                        with open(os.path.join(loc, Config.CONFIG_FILE_NAME)) as config_file:
                            self._conf = json.load(config_file)
                            self._config_file_path = os.path.join(loc, Config.CONFIG_FILE_NAME)
                            # convert string to boolean
                            self.set_system_debug_on(self.get_value(Config.CFG_KEY_SYSTEM_DEBUG_ON))
                        if self._conf is not None:
                            break
                except IOError:
                    pass

            if self._conf is None:
                print('Failed to load configuraiton file config.json.')
                self._conf = dict()
        else:
            self._conf = dict()
        # now enable auto save
        self._auto_save = auto_save

    def set_server_url(self, server_url):
        """
        Set  server URL
        Args:
            server_url: URL of server

        Returns:
            Config object

        """
        self.set_value(Config.CFG_KEY_SERVER_URL, server_url)
        return self

    def set_server_port(self, server_port):
        """
        Set server port
        Args:
            server_port: server port

        Returns:
            Config object

        """
        self.set_value(Config.CFG_KEY_SERVER_PORT, server_port)
        return self

    def set_system_debug_on(self, is_debug):
        """
        Set system debug
        Args:
            is_debug: boolean value

        Returns:
            Config object

        """
        # TODO Python2 hack
        if isinstance(is_debug, text):
            if is_debug.lower() in ['true', '1', 't', 'y', 'yes']:
                bool_debug_on = True
            else:
                bool_debug_on = False
        elif isinstance(is_debug, bool):
            bool_debug_on = is_debug
        else:
            raise Exception("Illegal input parameter type. Expect input parameter is boolean type or string type.")

        self.set_value(Config.CFG_KEY_SYSTEM_DEBUG_ON, bool_debug_on)

        return self

    def get_all(self):
        """
        Get all configuration key-value pair
        Returns: a dict containing configuration key-value pair

        """
        return self._conf.copy()

    def get_value(self, key):
        """
        Get configuration value from key
        Args:
            key: key defined in IRM_ prefix

        Returns: configuration value

        """
        if key in Config.READABLE_KEY_LIST:
            return self._conf.get(key, None)
        else:
            return None

    def set_value(self, key, value):
        """
        Set configuration (key, value)
        Args:
            key: key defined in IRM_ prefix
            value: config value
        """
        if key in Config.WRITABLE_KEY_LIST:
            self._conf[key] = value
        if self._auto_save:
            self.save_to_config_file()

    def save_to_config_file(self):
        """
        Save config to file

        """
        filter_dict = { k: v for k, v in self._conf.items() if k in Config.SAVE_KEY_LIST}
        with open(self._config_file_path, 'w') as outfile:
            json.dump(filter_dict, outfile, indent=4, sort_keys=True)


