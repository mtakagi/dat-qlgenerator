require 'rake/clean'

# rake clean で build ディレクトリ以下を削除
CLEAN.include("build")

desc "Install plugin"

task :install do
	system('xcodebuild -configuration Release -target "Install plugin for user"')
end

desc "Build for BathyScaphe"

task :bathyscaphe do
	system('xcodebuild -configuration Release -target "Build for BathyScaphe"')
end

desc "Build standard Quick Look plugin"

task :default do
	system('xcodebuild -configuration Release -target "dat"')
end
