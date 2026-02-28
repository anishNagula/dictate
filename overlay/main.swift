import AppKit
import Foundation

// MARK: - Waveform View

class WaveformView: NSView {

    private var amplitudes: [CGFloat] = Array(repeating: 0.05, count: 28)
    private var targetAmplitudes: [CGFloat] = Array(repeating: 0.05, count: 28)

    private let smoothing: CGFloat = 0.25
    private var animationTimer: Timer?

    override var isOpaque: Bool { false }

    override func viewDidMoveToWindow() {
        super.viewDidMoveToWindow()

        // 60 FPS animation loop
        animationTimer = Timer.scheduledTimer(
            timeInterval: 1.0 / 60.0,
            target: self,
            selector: #selector(step),
            userInfo: nil,
            repeats: true
        )
    }

    @objc private func step() {
        for i in 0..<amplitudes.count {
            amplitudes[i] += (targetAmplitudes[i] - amplitudes[i]) * smoothing
        }
        needsDisplay = true
    }

    func pushAmplitude(_ value: CGFloat) {
        targetAmplitudes.removeFirst()

        let scaled = min(max(value * 6, 0.0), 1.0)
        targetAmplitudes.append(scaled)
    }

    override func draw(_ dirtyRect: NSRect) {
        guard let ctx = NSGraphicsContext.current?.cgContext else { return }

        ctx.clear(bounds)

        let barWidth: CGFloat = 2
        let spacing: CGFloat = 3
        let totalWidth = CGFloat(amplitudes.count) * (barWidth + spacing)
        let horizontalPadding: CGFloat = 10

        var x = max(horizontalPadding,
                    (bounds.width - totalWidth) / 2)

        for amp in amplitudes {
            let height = max(4, amp * (bounds.height - 18))
            let y = (bounds.height - height) / 2

            let rect = CGRect(x: x, y: y, width: barWidth, height: height)

            ctx.setFillColor(NSColor.white.cgColor)
            ctx.fill(rect)

            x += barWidth + spacing
        }
    }
}

// MARK: - Overlay App

class OverlayApp: NSObject, NSApplicationDelegate {

    var window: NSWindow!
    var waveform: WaveformView!
    var process: Process?

    func applicationDidFinishLaunching(_ notification: Notification) {
        createWindow()
        startEngine()
    }

    func createWindow() {
        let screen = NSScreen.main!.frame

        let width: CGFloat = 200    // notch-like width
        let height: CGFloat = 50

        window = NSWindow(
        contentRect: NSRect(
            x: (screen.width - width) / 2,
            y: (screen.height - height) / 2,
            width: width,
            height: height
        ),
        styleMask: [.borderless],
            backing: .buffered,
            defer: false
        )

        window.isOpaque = false
        window.backgroundColor = .clear
        window.level = .floating
        window.ignoresMouseEvents = true
        window.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]

        // Black pill background
        let container = NSView(frame: window.contentView!.bounds)
        container.wantsLayer = true
        container.layer?.backgroundColor = NSColor.black.withAlphaComponent(0.9).cgColor
        container.layer?.cornerRadius = height / 2
        container.autoresizingMask = [.width, .height]

        waveform = WaveformView(frame: container.bounds)
        waveform.autoresizingMask = [.width, .height]

        container.addSubview(waveform)
        window.contentView?.addSubview(container)

        window.makeKeyAndOrderFront(nil)
    }

    func startEngine() {

        process = Process()

        process!.executableURL = URL(fileURLWithPath: "/usr/local/bin/dictate")
        process!.arguments = []

        // Explicit environment
        process!.environment = ProcessInfo.processInfo.environment

        let pipe = Pipe()
        process!.standardOutput = pipe
        process!.standardError = pipe

        pipe.fileHandleForReading.readabilityHandler = { handle in
            let data = handle.availableData
            if data.isEmpty { return }

            guard let output = String(data: data, encoding: .utf8) else { return }

            for line in output.split(separator: "\n") {

                if line.hasPrefix("AMP:") {
                    let valueStr = line.replacingOccurrences(of: "AMP:", with: "")
                    if let val = Double(valueStr) {
                        self.waveform.pushAmplitude(CGFloat(val))
                    }
                }

                if line == "DONE" {
                    DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
                        NSApplication.shared.terminate(nil)
                    }
                }
            }
        }

        do {
            try process!.run()
        } catch {
            print("Failed to start dictate:", error)
        }
    }
}

let app = NSApplication.shared
let delegate = OverlayApp()
app.delegate = delegate
app.setActivationPolicy(.accessory)
app.run()