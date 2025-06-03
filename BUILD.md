# ビルド手順

## 必要な環境

### Windows
- Visual Studio 2019以降
- CMake 3.16以降
- Git

### 必要なライブラリ
- **OBS Studio** (ソースコードが必要)
- **OpenCV 4.5+**
- **miniaudio** (ヘッダーオンリーライブラリ)

## セットアップ手順

### 1. OBS Studioのソースコードを取得

```bash
git clone --recursive https://github.com/obsproject/obs-studio.git
cd obs-studio
mkdir build
cd build
cmake ..
cmake --build . --config RelWithDebInfo
```

### 2. OpenCVのインストール

#### 方法A: vcpkgを使用（推奨）
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install opencv4[contrib]:x64-windows
```

#### 方法B: 公式サイトからダウンロード
1. [OpenCV公式サイト](https://opencv.org/releases/)からWindows版をダウンロード
2. 適当な場所に展開
3. 環境変数`OpenCV_DIR`を設定

### 3. このプラグインのビルド

```bash
git clone https://github.com/jokerjp1234/obs-game-audio-trigger.git
cd obs-game-audio-trigger
mkdir build
cd build

# CMakeの設定（パスは環境に合わせて調整）
cmake .. -DOBS_STUDIO_DIR="C:/path/to/obs-studio" \
         -DOpenCV_DIR="C:/path/to/opencv/build" \
         -DCMAKE_BUILD_TYPE=RelWithDebInfo

# ビルド実行
cmake --build . --config RelWithDebInfo
```

### 4. インストール

```bash
cmake --install . --config RelWithDebInfo
```

生成された`.dll`ファイルをOBS Studioのプラグインディレクトリにコピー:
- デフォルト: `%ProgramFiles%/obs-studio/obs-plugins/64bit/`
- Portable版: `obs-studio-portable/obs-plugins/64bit/`

## トラブルシューティング

### よくあるエラー

#### CMakeでOBSが見つからない
```
Could NOT find OBS (missing: OBS_INCLUDE_DIR)
```
**解決方法**: `-DOBS_STUDIO_DIR`パラメータでOBSソースコードのパスを正しく指定

#### OpenCVが見つからない
```
Could NOT find OpenCV (missing: OpenCV_DIR)
```
**解決方法**: `-DOpenCV_DIR`パラメータでOpenCVのビルドディレクトリを指定

#### リンクエラー
```
unresolved external symbol
```
**解決方法**: 
- OBSとプラグインのビルド設定（Debug/Release）を合わせる
- Visual Studio 2019以降を使用

### デバッグビルド

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DOBS_STUDIO_DIR="..." -DOpenCV_DIR="..."
cmake --build . --config Debug
```

## 開発環境の推奨設定

### Visual Studio Code設定例

`.vscode/settings.json`:
```json
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "cmake.configureSettings": {
        "OBS_STUDIO_DIR": "C:/path/to/obs-studio",
        "OpenCV_DIR": "C:/path/to/opencv/build"
    }
}
```

### 推奨拡張機能
- C/C++
- CMake Tools
- GitLens
