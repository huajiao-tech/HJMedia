import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';
import {
  HJPlayerSourceType,
  HJPlayerType,
  HJPlayerVideoCodecType,
  OpenPlayerInfo,
  serializeOpenPlayerInfo,
} from '../main/ets/HJPlayerTypes';

export default function localUnitTest() {
  describe('localUnitTest', () => {
    // Defines a test suite. Two parameters are supported: test suite name and test suite function.
    beforeAll(() => {
      // Presets an action, which is performed only once before all test cases of the test suite start.
      // This API supports only one parameter: preset action function.
    });
    beforeEach(() => {
      // Presets an action, which is performed before each unit test case starts.
      // The number of execution times is the same as the number of test cases defined by **it**.
      // This API supports only one parameter: preset action function.
    });
    afterEach(() => {
      // Presets a clear action, which is performed after each unit test case ends.
      // The number of execution times is the same as the number of test cases defined by **it**.
      // This API supports only one parameter: clear action function.
    });
    afterAll(() => {
      // Presets a clear action, which is performed after all test cases of the test suite end.
      // This API supports only one parameter: clear action function.
    });
    it('assertContain', 0, () => {
      // Defines a test case. This API supports three parameters: test case name, filter parameter, and test case function.
      let a = 'abc';
      let b = 'b';
      // Defines a variety of assertion methods, which are used to declare expected boolean conditions.
      expect(a).assertContain(b);
      expect(a).assertEqual(a);
    });

    it('serializeOpenPlayerInfo should keep sei uuid map entries in json string', 0, () => {
      let open_player_info: OpenPlayerInfo = {
        url: 'https://example.com/live.flv',
        localDir: '/data/storage/el2/base/haps/entry/files/hjmedias',
        rid: 'rid-test',
        fps: 30,
        repeats: 0,
        videoCodecType: HJPlayerVideoCodecType.HJPlayerVideoCodecType_OHCODEC,
        sourceType: HJPlayerSourceType.HJPlayerSourceType_SERIES,
        playerType: HJPlayerType.HJPlayerType_LIVESTREAM,
        bSplitScreenMirror: false,
        disableMFlag: 0,
        m_graphConfig: '',
        m_enableSEIUUids: new Map<string, boolean>([
          ['12345678123456781234567812345678', true],
          ['12345678123456781234567812345679', false],
        ]),
      };

      let raw_serialized = JSON.stringify(open_player_info);
      expect(raw_serialized.indexOf('12345678123456781234567812345678') >= 0).assertEqual(false);

      let serialized_open_player_info = serializeOpenPlayerInfo(open_player_info);
      expect(serialized_open_player_info).assertContain('"url":"https://example.com/live.flv"');
      expect(serialized_open_player_info).assertContain('"playerType":1');
      expect(serialized_open_player_info).assertContain('"12345678123456781234567812345678":true');
      expect(serialized_open_player_info).assertContain('"12345678123456781234567812345679":false');
    });
  });
}
