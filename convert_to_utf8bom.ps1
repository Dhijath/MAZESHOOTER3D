# convert_to_utf8bom.ps1
# .h / .cpp を全て UTF-8 BOM に変換する
#
# ルール:
#   1. 今セッションで書き直した確実なUTF-8ファイル → BOM なしなら付加、あればスキップ
#   2. BOM付きファイルのBOM後バイト列が正しいUTF-8 → スキップ（本物のUTF-8 BOM）
#   3. BOM付きだがBOM後バイト列が不正なUTF-8 → 偽BOM（Shift-JIS）→ SJIS変換して保存
#   4. BOM無しファイル → Shift-JISとして読み直しUTF-8 BOMで保存

$dir  = $PSScriptRoot
$sjis = [System.Text.Encoding]::GetEncoding('shift_jis')
$utf8bom = New-Object System.Text.UTF8Encoding($true)   # BOM 付き UTF-8
$utf8strict = New-Object System.Text.UTF8Encoding($false, $true) # BOM なし, 例外あり

# 今セッションで書き直した確実なUTF-8ファイル
$utf8Files = @(
    'Enemy.h', 'Enemy.cpp',
    'EnemyAI.h', 'EnemyAI.cpp',
    'EnemyTank.h', 'EnemyTank.cpp',
    'EnemySpeed.h', 'EnemySpeed.cpp',
    'EnemySniper.h', 'EnemySniper.cpp',
    'EnemyManager.cpp',
    'EnemyBullet.h', 'EnemyBullet.cpp',
    'EnemyAPI.h'
)

$converted = 0
$skipped   = 0
$errors    = 0

$files = Get-ChildItem -Path $dir -File | Where-Object { $_.Extension -in '.h','.cpp' }

foreach ($f in $files) {
    try {
        $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
        $hasBom = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)

        # 1. 今セッションで書き直したファイル
        if ($utf8Files -contains $f.Name) {
            if ($hasBom) {
                Write-Host "SKIP (my UTF-8 BOM): $($f.Name)"
                $skipped++
                continue
            }
            # BOMなし → BOM付加
            $text = [System.Text.Encoding]::UTF8.GetString($bytes)
            Write-Host "UTF-8 -> BOM: $($f.Name)"
            [System.IO.File]::WriteAllText($f.FullName, $text, $utf8bom)
            $converted++
            continue
        }

        if ($hasBom) {
            # BOM後のバイト列を厳密UTF-8で検証
            $content = $bytes[3..($bytes.Length - 1)]
            $isValidUtf8 = $true
            try {
                $utf8strict.GetString($content) | Out-Null
            } catch {
                $isValidUtf8 = $false
            }

            if ($isValidUtf8) {
                # 2. 本物のUTF-8 BOM → スキップ
                Write-Host "SKIP (valid UTF-8 BOM): $($f.Name)"
                $skipped++
                continue
            } else {
                # 3. 偽BOM（BOM + Shift-JIS） → SJIS変換
                $text = $sjis.GetString($content)
                Write-Host "FAKE-BOM(SJIS) -> UTF-8 BOM: $($f.Name)"
            }
        } else {
            # 4. BOM無し → Shift-JISとして変換
            $text = $sjis.GetString($bytes)
            Write-Host "SJIS -> UTF-8 BOM: $($f.Name)"
        }

        [System.IO.File]::WriteAllText($f.FullName, $text, $utf8bom)
        $converted++

    } catch {
        Write-Host "ERROR: $($f.Name) - $_"
        $errors++
    }
}

Write-Host ""
Write-Host "Done. Converted: $converted  Skipped: $skipped  Errors: $errors"
