# OBS Game Audio Trigger Plugin

🎮 ゲーム画面認識音楽再生OBSプラグイン

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://github.com/jokerjp1234/obs-game-audio-trigger)
[![OBS Studio](https://img.shields.io/badge/OBS%20Studio-28.0+-purple.svg)](https://obsproject.com/)

## 🌟 概要

このプラグインは、指定したゲームのプロセスを監視し、画面内で特定の画像を検出した時に自動的に音楽ファイルを再生するOBS Studioプラグインです。

### ✨ 主な機能

- 🔍 **プロセス監視**: 指定したゲーム（.exe名）のプロセスを自動検出
- 🖼️ **画像認識**: OpenCVを使用したリアルタイム画像マッチング
- 🎵 **音楽再生**: WAV/MP3ファイルの再生（音量・速度・時間制御対応）
- ⚙️ **詳細設定**: 認識感度、クールダウン時間など細かい調整が可能
- 🌐 **多言語対応**: 日本語・英語のUI
- 🐛 **デバッグ機能**: 詳細なログ出力とトラブルシューティング

## 🚀 使用例

- **レベルアップ時のファンファーレ再生**
- **アイテム取得時の効果音追加**
- **ボス撃破時の勝利音楽**
- **スコア達成時の祝福音**
- **ストリーミング配信での演出強化**

## 📋 技術仕様

- **言語**: C++17
- **画像認識**: OpenCV 4.5+
- **音声再生**: miniaudio
- **対応OS**: Windows (初期版)
- **OBS Studio**: 28.0+

## 🛠️ インストール

### バイナリリリース（推奨）

1. [Releases](https://github.com/jokerjp1234/obs-game-audio-trigger/releases)から最新版をダウンロード
2. DLLファイルをOBSプラグインフォルダにコピー:
   - デフォルト: `%ProgramFiles%/obs-studio/obs-plugins/64bit/`
   - Portable版: `obs-studio-portable/obs-plugins/64bit/`
3. OBS Studioを再起動

### ソースからビルド

詳細は [BUILD.md](BUILD.md) を参照してください。

## 📖 使用方法

詳細な使用方法は [USAGE.md](USAGE.md) を参照してください。

### クイックスタート

1. **ソース追加**: OBSで「Game Audio Trigger」ソースを追加
2. **プロセス設定**: 監視するゲームの実行ファイル名を入力（例: `game.exe`）
3. **画像設定**: 検出したい画面の画像ファイルを選択
4. **音声設定**: 再生したい音楽ファイルを選択
5. **調整**: マッチング閾値や音量などを調整

## 🎯 設定項目

### 基本設定
- ✅ **有効/無効**: プラグインの動作制御
- 🎮 **プロセス名**: 監視対象のゲーム実行ファイル
- 🖼️ **テンプレート画像**: 検出する画像ファイル（PNG/JPG）
- 🎵 **音声ファイル**: 再生する音楽ファイル（WAV/MP3/OGG）

### マッチング設定
- 🎯 **マッチング閾値**: 検出感度（0.0-1.0）
- ⏰ **クールダウン時間**: 連続再生防止の待機時間

### 音声設定
- 🔊 **音量**: 再生音量（0.0-1.0）
- ⚡ **再生速度**: 再生速度（0.1-3.0倍）
- ⏱️ **再生時間**: 再生時間制限（-1で全体再生）

## 🔧 トラブルシューティング

### よくある問題

| 問題 | 解決方法 |
|------|----------|
| 音が鳴らない | プロセス名、画像ファイル、音声ファイルを確認 |
| 誤検出が多い | マッチング閾値を上げる（0.8→0.9） |
| 検出されない | マッチング閾値を下げる（0.8→0.6） |
| 連続再生される | クールダウン時間を長くする |

詳細なトラブルシューティングは [USAGE.md](USAGE.md) を参照してください。

## 🤝 コントリビューション

プロジェクトへの貢献を歓迎します！

### 貢献方法
1. このリポジトリをフォーク
2. 新しいブランチを作成 (`git checkout -b feature/new-feature`)
3. 変更をコミット (`git commit -am 'Add new feature'`)
4. ブランチにプッシュ (`git push origin feature/new-feature`)
5. プルリクエストを作成

### 報告・要望
- 🐛 [バグ報告](https://github.com/jokerjp1234/obs-game-audio-trigger/issues)
- 💡 [機能要望](https://github.com/jokerjp1234/obs-game-audio-trigger/issues)
- 📝 [ディスカッション](https://github.com/jokerjp1234/obs-game-audio-trigger/discussions)

## 📄 ライセンス

このプロジェクトは [MIT License](LICENSE) の下で公開されています。

## 🙏 謝辞

- [OBS Studio](https://obsproject.com/) - 配信ソフトウェア
- [OpenCV](https://opencv.org/) - 画像認識ライブラリ
- [miniaudio](https://miniaud.io/) - 音声再生ライブラリ

## 📊 プロジェクト状態

- ✅ 基本機能実装
- ✅ Windows対応
- 🔄 テスト中
- 🔄 ドキュメント整備
- 📋 Linux/macOS対応（予定）
- 📋 GUI改善（予定）

## 🔗 関連リンク

- [OBS Studio](https://obsproject.com/)
- [OpenCV Documentation](https://docs.opencv.org/)
- [CMake](https://cmake.org/)

---

⭐ このプロジェクトが役に立ったら、ぜひスターをお願いします！
