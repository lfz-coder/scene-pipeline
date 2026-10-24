// A deterministic, silent 3-second test movie; no downloaded assets or FFmpeg needed.
import AVFoundation
import CoreGraphics
import CoreVideo
import Foundation

let output = URL(fileURLWithPath: CommandLine.arguments[1])
if FileManager.default.fileExists(atPath: output.path) { try FileManager.default.removeItem(at: output) }
let writer = try AVAssetWriter(outputURL: output, fileType: .mov)
let input = AVAssetWriterInput(mediaType: .video, outputSettings: [
    AVVideoCodecKey: AVVideoCodecType.h264,
    AVVideoWidthKey: 320,
    AVVideoHeightKey: 180
])
let adaptor = AVAssetWriterInputPixelBufferAdaptor(assetWriterInput: input, sourcePixelBufferAttributes: [
    kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA,
    kCVPixelBufferWidthKey as String: 320,
    kCVPixelBufferHeightKey as String: 180
])
input.expectsMediaDataInRealTime = false
writer.add(input)
guard writer.startWriting() else { fatalError("Cannot start video writer: \(String(describing: writer.error))") }
writer.startSession(atSourceTime: .zero)
for index in 0..<45 {
    while !input.isReadyForMoreMediaData { Thread.sleep(forTimeInterval: 0.005) }
    var buffer: CVPixelBuffer?
    CVPixelBufferCreate(kCFAllocatorDefault, 320, 180, kCVPixelFormatType_32BGRA, nil, &buffer)
    guard let buffer = buffer else { fatalError("Cannot allocate video frame") }
    CVPixelBufferLockBaseAddress(buffer, [])
    let context = CGContext(data: CVPixelBufferGetBaseAddress(buffer), width: 320, height: 180,
        bitsPerComponent: 8, bytesPerRow: CVPixelBufferGetBytesPerRow(buffer),
        space: CGColorSpaceCreateDeviceRGB(), bitmapInfo:
        CGImageAlphaInfo.premultipliedFirst.rawValue | CGBitmapInfo.byteOrder32Little.rawValue)!
    context.setFillColor(red: CGFloat(index) / 45, green: 0.25, blue: 0.7, alpha: 1)
    context.fill(CGRect(x: 0, y: 0, width: 320, height: 180))
    context.setFillColor(red: 0.9, green: 0.8, blue: 0.2, alpha: 1)
    context.fill(CGRect(x: index * 5, y: 55, width: 60, height: 70))
    CVPixelBufferUnlockBaseAddress(buffer, [])
    guard adaptor.append(buffer, withPresentationTime: CMTime(value: Int64(index), timescale: 15))
        else { fatalError("Cannot append video frame") }
}
input.markAsFinished()
let done = DispatchSemaphore(value: 0)
writer.finishWriting { done.signal() }
done.wait()
guard writer.status == .completed else { fatalError("Video failed: \(String(describing: writer.error))") }
print("Created \(output.path)")
