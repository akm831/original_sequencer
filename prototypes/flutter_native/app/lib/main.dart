import 'package:flutter/material.dart';

void main() => runApp(const PrototypeApp());

class PrototypeApp extends StatelessWidget {
  const PrototypeApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      debugShowCheckedModeBanner: false,
      home: PrototypeScreen(),
    );
  }
}

class PrototypeScreen extends StatelessWidget {
  const PrototypeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Sequencer Prototype')),
      body: const Center(
        child: Text('Flutter + Native Audio — P0 skeleton'),
      ),
    );
  }
}
