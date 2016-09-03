export CPUPROFILE=/tmp/scans.prof
make profile
./scans
google-pprof --dot ./scans /tmp/scans.prof > scans.dot
dot -Tsvg scans.dot > scans.svg
google-chrome scans.svg

rm scans.dot
