import 'dart:async';
import 'package:flutter_test/flutter_test.dart';
import '../lib/audio_focus_gate.dart';
void main() {
  test('late grant cannot start after stop or backgrounding',() async {
    final reply=Completer<bool>(); var releases=0;
    final focus=AudioFocusGate(request:()=>reply.future,abandon:() async { releases++; });
    final play=focus.acquire(); await focus.cancel(); reply.complete(true);
    expect(await play,false); expect(releases,2);
  });
  test('a stale play reply cannot release focus from a newer play',() async {
    final first=Completer<bool>(),second=Completer<bool>(); var calls=0,releases=0;
    final focus=AudioFocusGate(request:()=>calls++==0 ? first.future : second.future,
      abandon:() async { releases++; });
    final old=focus.acquire(),next=focus.acquire();
    first.complete(true); expect(await old,false); expect(releases,0);
    second.complete(true); expect(await next,true);
    await focus.cancel(); expect(releases,1);
  });
  test('dispose cancels a pending request and denial never plays',() async {
    final reply=Completer<bool>();
    final focus=AudioFocusGate(request:()=>reply.future,abandon:() async {});
    final play=focus.acquire(); focus.dispose(); reply.complete(true);
    expect(await play,false); expect(await focus.acquire(),false);
    final denied=AudioFocusGate(request:() async=>false,abandon:() async {});
    expect(await denied.acquire(),false);
  });
  test('platform failure is handled as denied focus',() async {
    final focus=AudioFocusGate(request:() async=>throw StateError('no channel'),
      abandon:() async=>throw StateError('no channel'));
    expect(await focus.acquire(),false); await focus.cancel();
  });
}
