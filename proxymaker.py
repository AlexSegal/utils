#!/bin/env python
######################################
# By Alex Segal, 2018
# Use and modify it on your own risk.
######################################

"""
A simple utility to make video proxies, presumably from 4K video files. 
Requires ffmpeg installed in the system.

    Usage: proxymaker <directory1 [directory2 directory3 ...]>

Recurse inside specified directories to locate all video files, convert them
to (lo-res) proxies. Save those proxies using identical file names, in an auto-created
subdirectory under each source directory, named proxy.XXXXxYYYY, where XXXX and YYYY are X,Y 
resolution of the generated proxy files.
"""

import os
import sys
import glob
import math
import subprocess
import logging
import optparse

# target format:
PROXY_SIZE = (1080, 720)
VIDEO_CODEC = 'libx264'
AUDIO_CODEC = 'copy'
CRF = 20  # Constant Rate Factor
          # the lower the better the video compression quality 
          # but the larger the file size.
          # 0 == lossless, 51 == the worst, 23 == ffmpeg default

VIDEO_EXTS = ('.mov', '.mp4', '.mks',) 
ONLY_OVERWRITE_IF_NEWER = False

DEST_DIR_PREFIX = 'proxy.'

log = logging.getLogger(os.path.splitext(os.path.basename(sys.argv[0]))[0])
log.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s %(name)s: [%(levelname)s] %(message)s')
ch.setFormatter(formatter)
log.addHandler(ch)

def makeTranscodeCmdLine(src, dst, exe='ffmpeg'):
    """Make a list of strings: ffmpeg command line for video/audio transcoding.
    """
    result = [exe]
    result.extend(['-hwaccel', 'dxva2'])
    result.extend(['-i', src])
    result.extend(['-c:v', VIDEO_CODEC])
    result.extend(['-c:a', AUDIO_CODEC])
    result.extend(['-crf', str(CRF)])
    result.extend(['-s', '%dx%d' % PROXY_SIZE])
    result.extend(['-y', '-threads', '0'])
    result.append(dst)
    return result

def transcode(src, dst, dryRun=False):
    """Run the command for video/audio transcoding.
    """
    log.info('*** Transcoding %s... ***' % src)
    
    cmd = makeTranscodeCmdLine(src, dst)

    log.debug('command: ' + ' '.join(cmd))
    
    if dryRun:
        return
        
    try:
        subprocess.check_call(cmd)
    except:
        if os.path.exists(dst):
            # partially saved files should be deleted!
            os.unlink(dst)
            log.warning('%s: partially saved file deleted!' % dst)
        raise
            
    if not os.path.exists(dst) or os.path.getsize(dst) == 0:
        raise ValueError('%s: destination file not found or empty after transcoding' % dst)
    
def transcodeFolder(dir, dryRun=False):
    """Recursively find and transcode all video files inside a folder and
    all of its subfolders. Save transcoded files in a subdirectory next to each file,
    named "proxy.XXXXxYYYY"
    Return # of files found and transcoded.
    """
    result = 0
    
    if not os.path.isabs(dir):
        dir = os.path.abspath(dir)

    destDirName = '%s%dx%d' % (DEST_DIR_PREFIX, PROXY_SIZE[0], PROXY_SIZE[1])
        
    for root, dirs, files in os.walk(dir):
        while dirs and dirs[0].startswith(DEST_DIR_PREFIX):
            dirs.pop(0)
        
        outDir = os.path.join(root, destDirName)
        
        for f in files:
            if os.path.splitext(f)[1].lower() not in VIDEO_EXTS:
                continue
            
            srcFile = os.path.join(root, f)
            dstFile = os.path.join(outDir, f)

            if ONLY_OVERWRITE_IF_NEWER and \
               os.path.exists(dstFile) and \
               os.path.getsize(dstFile) > 0 and \
               os.path.getmtime(dstFile) >= os.path.getmtime(srcFile):
                compression = float(os.path.getsize(srcFile)) / os.path.getsize(dstFile)
                log.info('Skipping %s: newer than the source. '
                         'Compression ratio: %02f' % (dstFile, compression))
                continue
                
            if not dryRun and not os.path.exists(outDir):
                os.makedirs(outDir)
            
            try:
                transcode(srcFile, dstFile, dryRun)
            except Exception, e:
                log.warning('%s: %e' % (srcFile, e))
            else:
                if os.path.exists(dstFile) and \
                   os.path.getsize(dstFile) > 0:
                    compression = float(os.path.getsize(srcFile)) / \
                                  os.path.getsize(dstFile)
                    log.info('%s: done. Compression ratio: %02f' % \
                             (dstFile, compression))
                    result += 1
                
    return result

if __name__ == '__main__':
    parser = optparse.OptionParser()

    parser.add_option('-d', '--dry-run', dest='dry_run',
                      action='store_true', default=False,
                      help='don\'t run any commands, just pretend')
    parser.add_option('-s', '--size',
                      action='store', dest='proxy_size', default=PROXY_SIZE,
                      type='int', nargs=2,
                      help='proxy resolution: X and Y, default: %d %d' % \
                           PROXY_SIZE)
    parser.add_option('-c', '--crf', dest='crf',
                      action='store', default=CRF, type='int',
                      help='Constand Bit Factor, default: %d' % CRF)
    parser.add_option('-n', '--newer', dest='newer',
                      action='store_true', default=ONLY_OVERWRITE_IF_NEWER,
                      help='skip existing destination files in case they '
                           'are newer than the sources, default: %s' % \
                           ONLY_OVERWRITE_IF_NEWER)

    options, args = parser.parse_args()

    PROXY_SIZE = options.proxy_size
    CRF = options.crf
    ONLY_OVERWRITE_IF_NEWER = options.newer
        
    if not args:
        raise ValueError('No directories specified')
        
    fileCount = 0
    
    for dir in args:
        if not os.path.isdir(dir):
            log.warning('%s is not found, skipped' % dir)
            
        try:
            fileCount += transcodeFolder(dir, dryRun=options.dry_run)
        except Exception, e:
            log.warning('%s: %s' % (dir, e))
            
    log.info('Done. %d file(s) transcoded' % fileCount)
