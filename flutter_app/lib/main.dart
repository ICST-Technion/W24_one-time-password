import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';
import 'package:flutter_progress_hud/flutter_progress_hud.dart';
import 'statistics_page.dart';

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
  int _passwordLength = 3;
  int _passwordDuration = 1;
  String _defaultPassword = '';

  final TextEditingController _defaultPasswordController = TextEditingController();

  FlutterBluetoothSerial bluetooth = FlutterBluetoothSerial.instance;
  bool _isSendingSettings = false;
  bool _isBluetoothConnected= false;
  bool _acknowledgmentReceived = false;
  String _passwordReceived = '';
  List<List<int>>? _statisticsList = null;
  String _firstPart = '';
  bool _isShowingNotification = false;

  final GlobalKey<ScaffoldState> _scaffoldKey = GlobalKey<ScaffoldState>();
  BluetoothConnection? _bluetoothConnection;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
    key: _scaffoldKey,
    appBar: AppBar(
      title: Text('One Time Password'),
      backgroundColor: Colors.blue,
      actions: [
        // Added Bluetooth connection icon
        IconButton(
          icon: Icon(
            _isBluetoothConnected ? Icons.bluetooth : Icons.bluetooth_disabled,
            color: _isBluetoothConnected ? Colors.green : Colors.red,
          ),
            onPressed: _isShowingNotification ? null : () => attemptBluetoothConnection(context),
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
                SizedBox(height: 20),
                Text('Set password settings:'),
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
                  onPressed: _isShowingNotification ||  _isSendingSettings ? null : () => sendSettings(context),
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
            if (_isShowingNotification == false) {
              getPassword(context);
            }
          } else if (index == 1) {
            if (_isShowingNotification == false) {
              getStatistics(context);
            }
          }
        },
      ),
    );
  }

  void showSnackBar(BuildContext context, String message, int duration) {
    setState(() {
      _isShowingNotification = true;
    });

    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(
          message,
          textAlign: TextAlign.center,
          style: TextStyle(fontSize: 20.0), // Adjust the font size
        ),
        duration: Duration(seconds: duration),
        behavior: SnackBarBehavior.floating, // Set behavior to floating
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(10.0),
        ),
      ),
    ).closed.then((reason) {
      // Called when the notification is dismissed
      setState(() {
        _isShowingNotification = false;
      });
    });
  }

  void getPassword(BuildContext context) async {
    final progress = ProgressHUD.of(context);
    progress?.show(); // Show loading indicator

    if (_isBluetoothConnected == false) {
      showSnackBar(context, 'Bluetooth device disconnected. Press the Bluetooth icon to try to reconnect.', 5);
      progress?.dismiss(); // Dismiss loading indicator
      return;
    }

    try {
      print('Connected to the device, requesting the password...');

      String password = 'password';
      _bluetoothConnection?.output.add(Uint8List.fromList(utf8.encode(password)));
      await _bluetoothConnection?.output.allSent;

      // Send settings and wait for acknowledgment with timeout
      int timeoutInSeconds = 15;
      DateTime startTime = DateTime.now();
      while (_passwordReceived.isEmpty) {
        // Check if timeout has occurred
        if (!_isBluetoothConnected ||
            DateTime.now().difference(startTime).inSeconds >= timeoutInSeconds) {
          // Timeout occurred or disconnected
          if (!_isBluetoothConnected) {
            showSnackBar(context, 'Bluetooth device disconnected. Press the Bluetooth icon to try to reconnect.', 5);
          } else {
            showSnackBar(context, 'Timeout occurred while waiting for password. Please try again.', 5);
          }
          return;
        }
        // Wait for a short interval before checking again
        await Future.delayed(Duration(seconds: 1));
      }

      showSnackBar(context, 'Current password is: $_passwordReceived', 10);
      _passwordReceived = '';
      print('Got password.');
    } catch (e) {
      print('Failed to get the password: $e');
      progress?.dismiss(); // Dismiss loading indicator
      showSnackBar(context, 'Bluetooth device disconnected. Press the Bluetooth icon to try to reconnect.', 5);
      setState(() {
        _isBluetoothConnected = false;
      });
    } finally {
      progress?.dismiss(); // Always dismiss the progress indicator
    }
  }

  void getStatistics(BuildContext context) async {
    final progress = ProgressHUD.of(context);
    progress?.show(); // Show loading indicator

    if (_isBluetoothConnected == false) {
      showSnackBar(context, 'Bluetooth device disconnected. Press the Bluetooth icon to try to reconnect.', 5);
      progress?.dismiss(); // Dismiss loading indicator
      return;
    }

    try {
      print('Connected to the device, requesting the statistics...');

      String statistics = 'statistics';
      _bluetoothConnection?.output.add(Uint8List.fromList(utf8.encode(statistics)));
      await _bluetoothConnection?.output.allSent;

      int timeoutInSeconds = 15;
      DateTime startTime = DateTime.now();
      while (_statisticsList == null) {
        // Check if timeout has occurred
        if (!_isBluetoothConnected ||
            DateTime.now().difference(startTime).inSeconds >= timeoutInSeconds) {
          // Timeout occurred or disconnected
          if (!_isBluetoothConnected) {
            showSnackBar(context, 'Bluetooth device disconnected. Press the Bluetooth icon to try to reconnect.', 5);
          } else {
            showSnackBar(context, 'Timeout occurred while waiting for statistics. Please try again.', 5);
          }
          progress?.dismiss();
          return;
        }
        // Wait for a short interval before checking again
        await Future.delayed(Duration(seconds: 1));
      }

      // Check if _statisticsList is not null before using it
      if (_statisticsList != null) {
        List<List<int>> statisticsCopy = List.from(_statisticsList!);
        Navigator.push(
          context,
          MaterialPageRoute(builder: (context) => StatisticsPage(statistics: statisticsCopy)),
        );
      }

      _statisticsList = null;
      print('Got statics.');
    } catch (e) {
      print('Failed to get the statistics: $e');
      progress?.dismiss(); // Dismiss loading indicator
      showSnackBar(context, 'Bluetooth device disconnected. Press the Bluetooth icon to try to reconnect.', 5);
      setState(() {
        _isBluetoothConnected = false;
      });
    } finally {
      progress?.dismiss(); // Always dismiss the progress indicator
    }
  }

  void attemptBluetoothConnection(BuildContext context) async {
    if (_isBluetoothConnected == false) {
      final progress = ProgressHUD.of(context);
      progress?.show(); // Show loading indicator

      List<BluetoothDevice> devices = [];
      try {
        devices = await bluetooth.getBondedDevices();
      } catch (e) {
        print("Error getting bonded devices: $e");
        showSnackBar(context, 'Bluetooth error: Failed to retrieve paired devices. Please ensure Bluetooth is enabled and try again.', 5);
        progress?.dismiss();
        setState(() {
          _isBluetoothConnected = false;
        });
        return;
      }

      BluetoothDevice? targetDevice;
      for (var device in devices) {
        if (device.name == 'ESP32') {
          targetDevice = device;
          break; // Exit the loop once the target device is found
        }
      }

      if (targetDevice != null) {
        try {
          final connection = await BluetoothConnection.toAddress(targetDevice.address);
          setState(() {
            _isBluetoothConnected = true;
            _bluetoothConnection = connection;
            print('Connected to the ESP32 device');
          });
          showSnackBar(context, 'Successfully connected to Bluetooth device.', 5);

          connection.input!.listen((data) => handleReceivedData(context, data),
              onDone: () {
            // Handle disconnection
            print('Disconnected by remote request');
            setState(() {
              _isBluetoothConnected = false;
            });
          }, onError: (error) {
            // Handle error
            print('Data reception error: $error');
            setState(() {
              _isBluetoothConnected = false;
            });
          });
          progress?.dismiss();
          return;
        } catch (e) {
          print("Failed to connect: $e");
          showSnackBar(context, 'Bluetooth error: Failed to connect to ESP32 device. Please ensure it is turned on and try again.', 5);
          setState(() {
            _isBluetoothConnected = false;
          });
        }
      } else {
        print('ESP32 device not found');
        showSnackBar(context, 'Bluetooth error: ESP32 device not found among paired devices.', 5);
        setState(() {
          _isBluetoothConnected = false;
        });
      }

      // Additional error handling...
      progress?.dismiss(); // Ensure loading indicator is dismissed in all cases
      return;
    }
  }

  void handleReceivedData(BuildContext context, Uint8List data) async {
    String receivedData = utf8.decode(data);
    // Handle received data
    print('Received data: $receivedData\n');

    // Example of handling specific acknowledgments globally
    if(receivedData.contains('ack')) {
      setState(() {
        _acknowledgmentReceived = true;
      });
    } else if (receivedData.contains('time')) {
      DateTime now = DateTime.now();
      String formattedTime = now.toIso8601String();
      if (_isBluetoothConnected != null) {
        _bluetoothConnection!.output.add(Uint8List.fromList(utf8.encode(formattedTime)));
        await _bluetoothConnection!.output.allSent;
        print('Current time: $formattedTime');
      }
    } else if (receivedData.contains('{') && (!receivedData.contains('}'))) {
      _firstPart = receivedData;
    } else if (receivedData.contains('}')) {
      receivedData = _firstPart + receivedData;
      receivedData = receivedData.substring(1, receivedData.length - 1);
      _statisticsList = decodeListOfLists(receivedData);
      _firstPart = '';
    } else if (receivedData.contains('lock')) {
      showSnackBar(context, 'Door is locked for 30 seconds due to multiple failed attempts.', 5);
    } else if (receivedData.contains('disconnected')) {
      showSnackBar(context, 'Keypad unit has disconnected.', 5);
    } else if (receivedData.contains('reconnected')) {
      showSnackBar(context, 'Keypad unit has reconnected.', 5);
    } else {
      print('Received password: $receivedData');
      _passwordReceived = receivedData;
    }
  }

  List<List<int>> decodeListOfLists(String jsonString) {
    List<dynamic> decodedList = jsonDecode(jsonString);
    List<List<int>> listOfLists = decodedList.map((list) => List<int>.from(list)).toList();
    return listOfLists;
  }

  void resetSettings() {
    setState(() {
      _passwordLength = 3;
      _passwordDuration = 1;
      _defaultPassword = '';
      _defaultPasswordController.clear();
      _isSendingSettings = false;
    });
  }

  void sendSettings(BuildContext context) async {
    setState(() {
      _isSendingSettings = true;
      _acknowledgmentReceived = false;
    });

    final progress = ProgressHUD.of(context);
    progress?.show(); // Show loading indicator

    if (_defaultPassword.isEmpty) {
      showSnackBar(context, 'Please enter a default password.', 4);
      progress?.dismiss(); // Dismiss loading indicator
      resetSettings();
      return;
    }

    // Validate default password
    if (!RegExp(r'^\d+$').hasMatch(_defaultPassword)) {
      showSnackBar(context, 'Default password must contain only digits.', 4);
      progress?.dismiss(); // Dismiss loading indicator
      resetSettings();
      return;
    }

    // Validate default password length
    if (_defaultPassword.length > _passwordLength) {
      showSnackBar(context, 'Default password length exceeds the chosen password length.', 4);
      progress?.dismiss(); // Dismiss loading indicator
      resetSettings();
      return;
    }

    if (_isBluetoothConnected == false) {
      showSnackBar(context, 'Bluetooth device disconnected. Press the Bluetooth icon to try to reconnect.', 5);
      progress?.dismiss(); // Dismiss loading indicator
      resetSettings();
      return;
    }

    try {
      print('Connected to the device, sending settings...');

      String settings = '$_passwordLength|$_passwordDuration|$_defaultPassword';
      _bluetoothConnection?.output.add(Uint8List.fromList(utf8.encode(settings)));
      await _bluetoothConnection?.output.allSent;

      // Send settings and wait for acknowledgment with timeout
      int timeoutInSeconds = 17;
      DateTime startTime = DateTime.now();
      while (!_acknowledgmentReceived) {
        // Check if timeout has occurred or Bluetooth is disconnected
        if (!_isBluetoothConnected ||
            DateTime.now().difference(startTime).inSeconds >= timeoutInSeconds) {
          // Timeout occurred or disconnected
          if (!_isBluetoothConnected) {
            showSnackBar(context, 'Bluetooth device disconnected. Press the Bluetooth icon to try to reconnect', 5);
          } else {
            showSnackBar(context, 'Timeout occurred while waiting for acknowledgment. Please try again.', 5);
          }
          resetSettings();
          return;
        }
        // Wait for a short interval before checking again
        await Future.delayed(Duration(seconds: 1));
      }


      showSnackBar(context, 'Successfully updated settings.', 6);
      _acknowledgmentReceived = false;
      print('Settings sent.');
    } catch (e) {
      print('Failed to send settings: $e');
      progress?.dismiss(); // Dismiss loading indicator
      showSnackBar(context, 'Bluetooth device disconnected. Press the Bluetooth icon to try to reconnect.', 6);
      resetSettings();
      setState(() {
        _isBluetoothConnected = false;
      });
    } finally {
      setState(() {
        _isSendingSettings = false;
      });
      progress?.dismiss(); // Always dismiss the progress indicator
      resetSettings();
    }
  }
}

