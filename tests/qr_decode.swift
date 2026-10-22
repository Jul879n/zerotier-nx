import CoreImage
import Foundation
let ci = CIImage(contentsOf: URL(fileURLWithPath: CommandLine.arguments[1]))!
let d = CIDetector(ofType: CIDetectorTypeQRCode, context: nil, options: [CIDetectorAccuracy: CIDetectorAccuracyHigh])!
for f in d.features(in: ci) { print((f as! CIQRCodeFeature).messageString ?? "nil") }
