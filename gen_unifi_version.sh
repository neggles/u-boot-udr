#!/bin/bash

HERE=`dirname $0`
HERE=`cd ${HERE};pwd`
BASEDIR=${HERE}
#OUTFILE=${BASEDIR}/build.properties

#get BRANCH and CUSTOMTAG from environment variables

#outtofile ()
#{
#   FILENAME=$1
#
#   cat > ${FILENAME} << EOF
#scm_reftype=${SCM_REFTYPE}
#scm_refname=${SCM_REFNAME}
#scm_distance=${SCM_DISTANCE}
#scm_hash=${SCM_HASH}
#scm_dirty=${SCM_DIRTY}
#build_date=${BUILD_DATE}
#build_time=${BUILD_TIME}
#build_user=${BUILD_USER}
#build_host=${BUILD_HOST}
#EOF
#}

pushd ${BASEDIR} > /dev/null
	BASE=v2018.03
    BUILD_USER=$(whoami)
    BUILD_HOST=$(hostname)
    BUILD_DATE=$(date +%F)
    BUILD_TIME=$(date +%T)
    GITDESCRIBE=$(git describe --dirty 2>/dev/null)
    DISTANCE=$(expr "${GITDESCRIBE}" : '.*-\([0-9]*\)-g.*')
    SHORTHASH=$(expr "${GITDESCRIBE}" : '.*-[0-9]*-g\([0-9a-f]*\).*')
    DIRTY=$(expr "${GITDESCRIBE}" : '.*-\(dirty\)$')
    LONGHASH=$(git rev-parse HEAD)
    
    SCM_HASH=${LONGHASH}
    
    # curent revision is an ATAG
    if [ "${BRANCH}" == "" -a "${GITDESCRIBE}" != "" -a "${DISTANCE}" == "" -a "${SHORTHASH}" == "" -a "${DIRTY}" == "" ]; then
        SCM_REFTYPE=atag
        TMP_REFNAME=$(echo ${GITDESCRIBE} | awk -F'-' '{print $1}')
        case ${TMP_REFNAME} in
        [0-9]*)
            SCM_REFNAME=$(echo v${TMP_REFNAME})
            ;;
        *)
            SCM_REFNAME=${TMP_REFNAME}
            ;;
        esac
        
        SCM_DISTANCE=$(expr "$(git describe --tags --match ${BASE})" : '.*-\([0-9]*\)-g.*')
        SCM_DIRTY=false
        HASH=$(git rev-parse HEAD | cut -c1-8)

		MY_SCM_REFNAME=$(echo ${SCM_REFNAME} | sed -e 's,/,_,g')
		UNIFI_REVISION=${MY_SCM_REFNAME}.${SCM_DISTANCE}
        if [ "${CUSTOMTAG}" != "" -a "${DIRTY}" != "" ]; then
            echo "unifi-${MY_SCM_REFNAME}.${SCM_DISTANCE}-g${HASH}-${CUSTOMTAG}"
			UNIFI_REVISION=${MY_SCM_REFNAME}.${SCM_DISTANCE}.${CUSTOMTAG}
			export UNIFI_REVISION
            return 0
        elif [ "${DIRTY}" == "" ]; then
            echo "unifi-${MY_SCM_REFNAME}.${SCM_DISTANCE}-g${HASH}"
            #outtofile ${OUTFILE}
			export UNIFI_REVISION
            return 0
        fi
		export UNIFI_REVISION
		return 0
	fi

    if [ "${BRANCH}" == "" ]; then 
        BRANCH=$(git branch | awk '/^\* /{print}' | sed 's,^\* ,,g')
        BRANCH_NOTEXIST=0
    else
        git show-ref --verify --quiet refs/heads/${BRANCH}
        BRANCH_NOTEXIST=$?
    fi

    if [ "${BRANCH}" == "(no branch)" -o "${BRANCH_NOTEXIST}" != "0" ]; then
        SCM_REFTYPE=unknown
        SCM_REFNAME=unknown
        SCM_DISTANCE=0
        DIRTY=$(expr "$(git describe --tags --dirty 2>/dev/null)" : '.*-\(dirty\)$')
        if [ "${DIRTY}" == "" ]; then
            SCM_DIRTY=false
        else
            SCM_DIRTY=true
        fi
        HASH=$(git rev-parse HEAD | cut -c1-8)

		MY_SCM_REFNAME=$(echo ${SCM_REFNAME} | sed -e 's,/,_,g')
        echo -n "unifi-${MY_SCM_REFNAME}.0-g${HASH}"
		UNIFI_REVISION=${MY_SCM_REFNAME}.0
        if [ "${DIRTY}" != "" ]; then
            if [ "${CUSTOMTAG}" != "" ]; then
                echo -n "-${CUSTOMTAG}"
				UNIFI_REVISION=${MY_SCM_REFNAME}.0.${CUSTOMTAG}
            else
                echo -n "-dirty"
            fi
        fi
        echo ""
        #outtofile ${OUTFILE}
		export UNIFI_REVISION
        return 0
    fi

    GITDESCRIBE=$(git describe --tags --dirty --match "${BASE}" 2>/dev/null)
    DISTANCE=$(expr "${GITDESCRIBE}" : '.*-\([0-9]*\)-g.*')
    SHORTHASH=$(expr "${GITDESCRIBE}" : '.*-[0-9]*-g\([0-9a-f]*\).*')
    DIRTY=$(expr "${GITDESCRIBE}" : '.*-\(dirty\)$')

    SCM_REFTYPE=branch
    SCM_REFNAME=${BRANCH}

    if [ "${GITDESCRIBE}" != "" ]; then
        # current revision is on a branch we are tracking
        if [ "${DISTANCE}" == "" -a "${SHORTHASH}" == "" ]; then
            SCM_DISTANCE=0
        else
            SCM_DISTANCE=${DISTANCE}
        fi
    else
        # current revision is on a branch we are not tracking
        SCM_DISTANCE=-1
        DIRTY=$(expr "$(git describe --tags --dirty 2>/dev/null)" : '.*-\(dirty\)$')
    fi

    if [ "${DIRTY}" == "" ]; then
        SCM_DIRTY=false
    else
        SCM_DIRTY=true
    fi
    HASH=$(git rev-parse HEAD | cut -c1-8)

	MY_SCM_REFNAME=$(echo ${SCM_REFNAME} | sed -e 's,/,_,g')
    echo -n "unifi-${MY_SCM_REFNAME}.${SCM_DISTANCE}-g${HASH}"
	UNIFI_REVISION=${MY_SCM_REFNAME}.${SCM_DISTANCE}
    if [ "${DIRTY}" != "" ]; then
        if [ "${CUSTOMTAG}" != "" ]; then
            echo -n "-${CUSTOMTAG}"
			UNIFI_REVISION=${MY_SCM_REFNAME}.${SCM_DISTANCE}.${CUSTOMTAG}
        else
            echo -n "-dirty"
        fi
    fi
    echo ""
	export UNIFI_REVISION
    #outtofile ${OUTFILE}
        
popd > /dev/null
