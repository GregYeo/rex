#!/bin/bash
set -e

TEST_ROOT=$1
RS_BINARY=$2

PROJECT_ROOT=$(pwd)
DOCKER_HOST=localhost

echo "Using binary: $RS_BINARY"

echo "Setup local project environment for the test"
TEST_DIR="/tmp/rs_test_env"
rm -rf $TEST_DIR && mkdir -p $TEST_DIR/sub
cat <<EOF > $TEST_DIR/rs.yaml
remote_host: $DOCKER_HOST
remote_port: 2222
remote_user: greg
remote_dir: /tmp
EOF

echo "Spin up SSH Server"
docker compose -f ${TEST_ROOT}/compose.test.yml up -d --wait
#trap "docker compose -f ${TEST_ROOT}/compose.test.yml down" EXIT # Ensure cleanup on fail

# We create a wrapper so 'rs' (which calls 'ssh') doesn't hang for a password
echo "Mock SSH Password handling for the runner"
export SSHPASS="password123"
echo '#!/bin/bash' > $TEST_DIR/ssh
echo 'sshpass -e /usr/bin/ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "$@"' >> $TEST_DIR/ssh
chmod +x $TEST_DIR/ssh

echo "Run the test"
cd $TEST_DIR/sub
export PATH=$TEST_DIR:$PATH



echo "================================"
echo "CASE: Touching file in remote"
echo "================================"

echo "When: Simple file touch command"
$RS_BINARY touch touch_result.txt

echo "Then: touch_result.txt file is found in remote_dir"
echo "Verify"
if docker exec remote-vm ls /tmp | grep -q "touch_result.txt"; then
   echo "PASSED"
else
  echo "FAILED"
  exit 99;
fi

echo "================================"
echo "CASE: Running Quote script"
echo "================================"

echo "When: Command is quoted and includes redirection"
$RS_BINARY "echo 'hello' > test1.txt"

echo "Then: test1.txt file is found in remote_dir"
if docker exec remote-vm ls /tmp | grep -q "test1.txt"; then
   echo "PASSED"
else
  echo "FAILED"
  exit 99;
fi

echo "Then: Content contains 'hello'"
if docker exec remote-vm cat /tmp/test1.txt | grep -q "hello"; then
   echo "PASSED"
else
  echo "FAILED"
  exit 99;
fi

echo "================================"
echo "CASE: Running task script"
echo "================================"
cat <<EOF >$TEST_DIR/task.sh
echo "hello" > script_result.txt
ls -la >> script_result.txt
EOF

echo "When: Command is streamed from stdin"
$RS_BINARY bash -s < $TEST_DIR/task.sh

echo "Then: script_result.txt file is found in remote_dir"
if docker exec remote-vm ls /tmp | grep -q "script_result.txt"; then
   echo "PASSED"
else
  echo "FAILED"
  exit 99;
fi

echo "Then: Content contains 'hello'"
if docker exec remote-vm cat /tmp/script_result.txt | grep -q "hello"; then
   echo "PASSED"
else
  echo "FAILED"
  exit 99;
fi

