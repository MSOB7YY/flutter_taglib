Pod::Spec.new do |s|
  s.name             = 'flutter_taglib'
  s.version          = '0.0.1'
  s.summary          = 'Flutter TagLib plugin iOS platform delegate.'
  s.description      = 'Handles iOS-specific folder picking and security-scoped bookmark permissions.'
  s.homepage         = 'https://github.com/axel10/flutter_taglib'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'Axel' => 'axel@example.com' }
  s.source           = { :path => '.' }
  s.source_files = 'flutter_taglib/Sources/flutter_taglib/**/*'
  s.dependency 'Flutter'
  s.platform = :ios, '14.0'
  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES', 'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'i386' }
  s.swift_version = '5.0'
end
