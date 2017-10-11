#!/usr/bin/env bash
DOCKER="sudo docker"
MACH="sysadmin"

run_n_enter() {
  $DOCKER run --rm -ti $MACH bash
# export TERRAFORM_BINARY="docker run --rm -ti -e TF_LOG \
#           -e AWS_ACCESS_KEY_ID -e AWS_SECRET_ACCESS_KEY -e AWS_DEFAULT_REGION \
#           -v ${HOME}/.aws:/root/.aws -v ${PWD}:/go \
#           --workdir "/go" hashicorp/terraform:${TERRAFORM_VERSION}"


}

# destroy
cleanup() {
  echo "cleaning up"
  $DOCKER stop $MACH

}

run_n_enter
cleanup
