# セットアップ手順書

## 🚀 完全版プラグインの実装が完成しました！

### 📁 現在の実装状況

✅ **完成したファイル:**
- `src/plugin-main.cpp` - プラグインエントリーポイント
- `src/game-audio-trigger.cpp` - メインロジック実装
- `src/game-audio-trigger.h` - メインロジックヘッダー
- `src/process-detector.cpp` - プロセス検出・ウィンドウキャプチャ実装
- `src/process-detector.h` - プロセス検出ヘッダー
- `src/image-matcher.cpp` - OpenCV画像認識実装
- `src/image-matcher.h` - 画像認識ヘッダー
- `src/audio-player.cpp` - miniaudio音声再生実装
- `src/audio-player.h` - 音声再生ヘッダー
- `src/miniaudio.h` - miniaudioプレースホルダー
- `CMakeLists.txt` - CMakeビルド設定

## 🔧 次に必要な作業

### 1. 実際のminiaudio.hを取得

現在のminiaudio.hはプレースホルダーです。実際のファイルをダウンロードしてください：

```bash
# プロジェクトディレクトリで実行
curl -o src/miniaudio.h https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h
```

または手動でダウンロード：
- https://github.com/mackron/miniaudio/blob/master/miniaudio.h
- ファイルを `src/miniaudio.h` として保存

### 2. 必要な依存関係のインストール

#### OpenCVのインストール
```bash
# vcpkgを使用（推奨）
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install opencv4[contrib]:x64-windows
```

#### OBS Studioソースコードの取得
```bash
git clone --recursive https://github.com/obsproject/obs-studio.git
cd obs-studio
mkdir build
cd build
cmake ..
cmake --build . --config RelWithDebInfo
```

### 3. プラグインのビルド

```bash
cd obs-game-audio-trigger
mkdir build
cd build

# CMakeの設定（パスは環境に合わせて調整）
cmake .. -DOBS_STUDIO_DIR="C:/path/to/obs-studio" ^
         -DOpenCV_DIR="C:/path/to/opencv/build" ^
         -DCMAKE_BUILD_TYPE=RelWithDebInfo

# ビルド実行
cmake --build . --config RelWithDebInfo
```

### 4. インストール

生成されたDLLファイルをOBSプラグインディレクトリにコピー：
- デフォルト: `%ProgramFiles%/obs-studio/obs-plugins/64bit/`
- Portable版: `obs-studio-portable/obs-plugins/64bit/`

## ⚠️ 重要な注意事項

### プラグインが表示されない場合の確認項目

1. **依存関係の確認**
   - OpenCV DLLがシステムPATHにあるか
   - Visual Studio Redistributableがインストールされているか

2. **ビルド設定の確認**
   - OBSとプラグインのビルド設定（Debug/Release）が一致しているか
   - 64bit版でビルドされているか

3. **OBSログの確認**
   - OBS起動時のログでプラグインエラーを確認
   - `%APPDATA%/obs-studio/logs/` の最新ログを確認

4. **ファイル配置の確認**
   - DLLファイルが正しいディレクトリにあるか
   - データフォルダ（locale）も正しく配置されているか

## 🐛 よくある問題と解決方法

### 問題1: "モジュールが見つかりません"
**解決方法**: 
- OpenCV DLLをOBSの実行ディレクトリまたはシステムPATHに追加
- Visual Studio Redistributableをインストール

### 問題2: "プラグインが読み込まれない"
**解決方法**:
- OBSログを確認してエラーメッセージを特定
- 依存関係DLLの不足を解消

### 問題3: "ビルドエラー"
**解決方法**:
- CMakeのパス設定を確認
- Visual Studio 2019以降を使用
- すべての依存関係が正しくインストールされているか確認

## 📞 サポート

問題が解決しない場合は、GitHubのIssueで報告してください：
- エラーメッセージの詳細
- 実行環境（Windows版、OBSバージョン等）
- 実行したビルド手順

---

これで完全版のOBS Game Audio Triggerプラグインの実装が完了しました！
ビルドと動作確認を行って、実際にゲーム画面認識による音楽再生を体験してください。