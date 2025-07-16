# 📄 requirements.md  
## Katyusha Neco AI - 要件定義

### 1. 🎮 機能要件
- [x] ボタン短押しでAI音声会話起動（OpenAI Realtime）
- [x] OpenAI Realtime APIによる音声認識・会話・音声合成の統合
- [x] 会話終了・無音時スリープ（デフォルト60秒・yaml設定可）
- [x] スリープ復帰はボタン短押し
- [x] Stack-chanアバター表示（AtomS3 LCD）
- [x] Neco LEDで会話中の状態を視覚的に表現

### 2. 🖥️ ソフトウェア要件
- [x] 開発環境：ESP-IDFベース
- [x] OpenAI Realtime SDK（https://github.com/m5stack/openai-realtime-embedded-sdk）
- [x] m5stack-avatar（https://github.com/stack-chan/m5stack-avatar）
- [x] 設定情報は外部yamlファイルに格納（API key / WiFi / パーソナリティ）

### 3. 📱 UI/UX要件
- [x] 表情連動：考え中・応答中は表情切り替え
- [x] リップシンク：mouthOpenRatioをベストエフォートで反映
- [x] LED（Neco）：会話時レインボー、待機時呼吸LED、スリープ時消灯

### 4. ⚙️ ハードウェア要件
- [x] AtomS3（M5Stack）
- [x] Echo Base（M5Stack）
- [x] Neco（Switch Science、LED70個搭載）
- [x] TailBat or 外部バッテリー
- [x] 音声I/OはEcho Base一体型

### 5. 🗂️ 設定ファイル例
```yaml
wifi:
  ssid: "your-ssid"
  password: "your-password"
openai:
  api_key: "sk-xxxxxxxxxx"
  voice: "alloy"
  personality: |
    あなたはスタックにゃんです。明るく親しみやすく話します。語尾に「にゃん」をつけてください。
sleep:
  timeout_sec: 60
```

### 6. 🎁 プロジェクト名と概要
- **プロジェクト名**：Katyusha Neco AI
- **説明**：AI猫耳カチューシャ。OpenAI Realtimeによるリアルタイム音声対話、スタックちゃんアバター、LED猫耳を搭載。