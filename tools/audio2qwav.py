import sys
import os
import subprocess
import struct
import argparse

def convert_to_qwav(input_path, output_path, sample_rate=None):
    # Get audio info first using ffprobe if sample_rate not provided
    if sample_rate is None:
        try:
            cmd = ['ffprobe', '-v', 'error', '-show_entries', 'stream=sample_rate', '-of', 'default=noprint_wrappers=1:nokey=1', input_path]
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode == 0:
                sample_rate = int(result.stdout.strip())
            else:
                sample_rate = 24000 # Fallback
        except:
            sample_rate = 24000

    # Get channels
    try:
        cmd = ['ffprobe', '-v', 'error', '-show_entries', 'stream=channels', '-of', 'default=noprint_wrappers=1:nokey=1', input_path]
        result = subprocess.run(cmd, capture_output=True, text=True)
        channels = int(result.stdout.strip()) if result.returncode == 0 else 1
    except:
        channels = 1

    print(f"Converting {input_path} to {output_path}...")
    print(f"Settings: {sample_rate}Hz, {channels} channel(s), 16-bit PCM")

    # Use ffmpeg to get raw pcm_s16le
    cmd = ['ffmpeg', '-i', input_path, '-ar', str(sample_rate), '-ac', str(channels), '-f', 's16le', '-acodec', 'pcm_s16le', 'pipe:1']
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    pcm_data, stderr = process.communicate()

    if process.returncode != 0:
        print("Error: FFmpeg failed.")
        print(stderr.decode())
        return

    # Write QWAV file
    with open(output_path, 'wb') as f:
        # Header (10 bytes)
        f.write(b'QWAV') # Magic
        f.write(struct.pack('<I', sample_rate)) # Sample rate (uint32)
        f.write(struct.pack('<B', channels)) # Channels (uint8)
        f.write(struct.pack('<B', 16)) # Bits per sample (uint8)
        # Data
        f.write(pcm_data)

    print(f"Done! Size: {len(pcm_data) + 10} bytes")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert audio files to .qwav format.')
    parser.add_argument('input', help='Input audio file (mp3, wav, etc.)')
    parser.add_argument('-o', '--output', help='Output .qwav file (default: same name as input)')
    parser.add_argument('-r', '--rate', type=int, help='Override sample rate')

    args = parser.parse_args()
    
    if not args.output:
        args.output = os.path.splitext(args.input)[0] + '.qwav'
        
    convert_to_qwav(args.input, args.output, args.rate)
