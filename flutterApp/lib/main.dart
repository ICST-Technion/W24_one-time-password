import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';
import 'package:flutter_progress_hud/flutter_progress_hud.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'One Time Password',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: ProgressHUD(
        child: MyHomePage(),
      ),
    );
  }
}

class MyHomePage extends StatefulWidget {
  @override
  _MyHomePageState createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  String _wifiSSID = '';
  String _wifiPassword = '';
  int _passwordLength = 3;
  int _passwordDuration = 1;
  String _defaultPassword = '';

  final TextEditingController _wifiSSIDController = TextEditingController();
  final TextEditingController _wifiPasswordController = TextEditingController();
  final TextEditingController _defaultPasswordController = TextEditingController();

  FlutterBluetoothSerial bluetooth = FlutterBluetoothSerial.instance;
  bool _isSendingSettings = false;
  bool _isConnectedToWifi= false;

  final GlobalKey<ScaffoldState> _scaffoldKey = GlobalKey<ScaffoldState>();

  @override
  Widget build(BuildContext context) {

    return Scaffold(
      key: _scaffoldKey,
      appBar: AppBar(
        title: Text('One Time Password'),
        backgroundColor: Colors.blue,
        actions: [
          IconButton(
            icon: Icon(
              _isConnectedToWifi ? Icons.wifi : Icons.wifi_off,
              color: _isConnectedToWifi ? Colors.green : Colors.black,
            ),
            onPressed: null, // Set onPressed to null to disable the button
          ),
        ],
      ),
      body: Center(
        child: SingleChildScrollView(
          child: Padding(
            padding: const EdgeInsets.all(20.0),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text('WiFi SSID:'),
                SizedBox(height: 20),
                TextField(
                  controller: _wifiSSIDController,
                  enabled: !_isSendingSettings,
                  onChanged: (value) {
                    setState(() {
                      _wifiSSID = value;
                    });
                  },
                  decoration: InputDecoration(
                    hintText: 'Enter WiFi SSID',
                  ),
                ),
                SizedBox(height: 20),
                Text('WiFi Password:'),
                TextField(
                  controller: _wifiPasswordController,
                  enabled: !_isSendingSettings,
                  onChanged: (value) {
                    setState(() {
                      _wifiPassword = value;
                    });
                  },
                  obscureText: true,
                  decoration: InputDecoration(
                    hintText: 'Enter WiFi Password',
                  ),
                ),
                SizedBox(height: 20),
                Text('Password Length: $_passwordLength'),
                Slider(
                  value: _passwordLength.toDouble(),
                  min: 3,
                  max: 6,
                  divisions: 3,
                  activeColor: Colors.blue,
                  onChanged: _isSendingSettings ? null : (value) {
                    setState(() {
                      _passwordLength = value.toInt();
                    });
                  },
                ),
                SizedBox(height: 20),
                Text('Password Duration (minutes): $_passwordDuration'),
                Slider(
                  value: _passwordDuration.toDouble(),
                  min: 1,
                  max: 60,
                  divisions: 59,
                  activeColor: Colors.blue,
                  onChanged: _isSendingSettings ? null : (value) {
                    setState(() {
                      _passwordDuration = value.toInt();
                    });
                  },
                ),
                SizedBox(height: 20),
                Text('Default Password (use only 0-9 digits):'),
                TextField(
                  controller: _defaultPasswordController,
                  enabled: !_isSendingSettings,
                  onChanged: (value) {
                    setState(() {
                      _defaultPassword = value;
                    });
                  },
                  obscureText: true,
                  decoration: InputDecoration(
                    hintText: 'Set Default Password',
                  ),
                ),
                SizedBox(height: 20),
                ElevatedButton(
                  onPressed: _isSendingSettings ? null : () => sendSettings(context),
                  child: Text(
                    'Save Settings',
                    style: TextStyle(color: Colors.blue),
                    ), // Set text color to white),
                ),
              ],
            ),
          ),
        ),
      ),
      bottomNavigationBar: BottomNavigationBar(
        items: const <BottomNavigationBarItem>[
          BottomNavigationBarItem(
            icon: Icon(Icons.lock),
            label: 'Password',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.bar_chart),
            label: 'Statistics',
          ),
        ],
        backgroundColor: Colors.blue,
        selectedItemColor: Colors.black,
        unselectedItemColor : Colors.black,
          onTap: (index) {
          if (index == 0) {
            Navigator.push(
              context,
              MaterialPageRoute(builder: (context) => PasswordPage()),
            );
          } else if (index == 1) {
            Navigator.push(
              context,
              MaterialPageRoute(builder: (context) => StatisticsPage()),
            );
          }
        },
      ),
    );
  }

  void resetSettings()
  {
    setState(() {
      _wifiSSID = '';
      _wifiPassword = '';
      _passwordLength = 3;
      _passwordDuration = 1;
      _defaultPassword = '';
      _wifiSSIDController.clear();
      _wifiPasswordController.clear();
      _defaultPasswordController.clear();
      _isSendingSettings = false;
    });
  }


  void sendSettings(BuildContext context) async {
    setState(() {
      _isSendingSettings = true;
    });

    final progress = ProgressHUD.of(context);
    progress?.show(); // Show loading indicator

    if (_wifiSSID.length == 0) {
      // Show notification for non-digit default password
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Please enter the wifi ssid'),
        ),
      );
      resetSettings();
      progress?.dismiss(); // Dismiss loading indicator
      return;
    }

    if (_wifiPassword.length == 0) {
      // Show notification for non-digit default password
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Please enter the wifi password'),
        ),
      );
      resetSettings();
      progress?.dismiss(); // Dismiss loading indicator
      return;
    }

    if (_defaultPassword.length == 0) {
      // Show notification for non-digit default password
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Please enter a default password'),
        ),
      );
      resetSettings();
      progress?.dismiss(); // Dismiss loading indicator
      return;
    }

    // Validate default password
    if (!RegExp(r'^\d+$').hasMatch(_defaultPassword)) {
      // Show notification for non-digit default password
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Default password must contain only digits'),
        ),
      );
      resetSettings();
      progress?.dismiss(); // Dismiss loading indicator
      return;
    }

    // Validate default password length
    if (_defaultPassword.length > _passwordLength) {
      // Show notification for default password length exceeding
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Default password length exceeds the maximum length'),
        ),
      );
      resetSettings();
      progress?.dismiss(); // Dismiss loading indicator
      return;
    }

    List<BluetoothDevice> devices = [];
    try {
      devices = await bluetooth.getBondedDevices();
    } catch (e) {
      print("Error getting bonded devices: $e");
      resetSettings();
      progress?.dismiss(); // Dismiss loading indicator
      // Show notification for Bluetooth connection error
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Failed to connect to Bluetooth device'),
        ),
      );
      return;
    }

    BluetoothDevice? esp32Device;
    for (var device in devices) {
      if (device.name == 'ESP32') {
        esp32Device = device;
        break;
      }
    }

    if (esp32Device != null) {
      BluetoothConnection? connection;
      try {
        connection = await BluetoothConnection.toAddress(esp32Device.address);
        print('Connected to the ESP32 device');

        String settings =
            '$_wifiSSID|$_wifiPassword|$_passwordLength|$_passwordDuration|$_defaultPassword';

        connection.output.add(Uint8List.fromList(utf8.encode(settings)));

        await connection.output.allSent;

        print('Settings sent to ESP32');

        final ackTimeout = Duration(seconds: 10);
        final ackReceived = await connection.input
            ?.where((data) => utf8.decode(data) == 'ack')
            ?.first
            ?.timeout(ackTimeout, onTimeout: () {
          print('Timeout: Ack message not received within 10 seconds');
          return Future.value(null); // Return a Future to indicate timeout with null value
        }) ??
            Future.value(null); // Default to null if null or if timeout occurs

        if (ackReceived != null) {
          print('Ack received');
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text('Successfully update settings'),
            ),
          );
        } else {
          print('Failed to receive ack message within 10 seconds');
          // Show notification for Bluetooth disconnection
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text('Device disconnected from Bluetooth'),
            ),
          );
        }

        await connection.finish();
        print('Connection closed');
      } catch (error) {
        print('Failed to connect');
        print(error.toString());
        // Show notification for Bluetooth connection error
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Failed to connect to Bluetooth device'),
          ),
        );
      } finally {
        if (connection != null) {
          await connection.close(); // Close the connection
        }
      }
    } else {
      print('ESP32 device not found');
      // Show notification for Bluetooth device not found
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('ESP32 device not found'),
        ),
      );
    }

    resetSettings();
    progress?.dismiss(); // Dismiss loading indicator
  }
}

class PasswordPage extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Password Page'),
      ),
      body: Center(),
    );
  }
}

class StatisticsPage extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Statistics Page'),
      ),
      body: Center(),
    );
  }
}
