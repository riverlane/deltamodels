node('linux') {
    stage('Checkout') {
        cleanWs()
        checkout scm
    }

    stage('Clean') {
        sh 'make clean'
    }

    stage('Build image and start container') {
        sh 'make container'
    }

    stage('Run cppcheck') {
        warnError('Error occured, catching exception and continuing to store test results.') {
            sh 'make cppcheck'
            archiveArtifacts artifacts: 'cppcheck.xml'
            publishCppcheck allowNoReport: true,
            pattern: 'cppcheck.xml'
        }
    }

    stage('Generate internal docs') {
        sh 'make docs-internal'
        publishHTML(
            [
                allowMissing: false,
                alwaysLinkToLastBuild: true,
                keepAll: false,
                reportDir: '',
                reportFiles: 'docs-internal/html/index.html',
                reportName: 'Doxygen-internal',
                reportTitles: ''
            ]
        )
    }

    stage('Generate public docs') {
        sh 'make docs'
        archiveArtifacts artifacts: 'docs/sphinx-build-html.log, docs/sphinx-build-pdf.log, docs/sphinx-build-epub.log'
        publishHTML(
            [
                allowMissing: false,
                alwaysLinkToLastBuild: true,
                keepAll: false,
                reportDir: '',
                reportFiles: 'docs/_build/html/index.html',
                reportName: 'Doxygen',
                reportTitles: ''
            ]
        )
    }

    stage('Run the tests') {
        warnError('Error occured, catching exception and continuing to store test results.') {
            sh 'make tests'
        }
    }

    stage('Publish results') {
        xunit(
            [
                CTest(
                    deleteOutputFiles: true,
                    failIfNotNew: true,
                    pattern: 'build/Testing/**/Test.xml',
                    skipNoTestFiles: false,
                    stopProcessingIfError: true
                )
            ]
        )
    }

    stage('Extract Coverage') {
        sh 'make coverage'
        archiveArtifacts artifacts: 'coverage.xml'
        cobertura autoUpdateHealth: false,
        autoUpdateStability: false,
        coberturaReportFile: 'coverage.xml',
        conditionalCoverageTargets: '70, 0, 0',
        failUnhealthy: false,
        failUnstable: false,
        lineCoverageTargets: '80, 0, 0',
        maxNumberOfBuilds: 0,
        methodCoverageTargets: '80, 0, 0',
        onlyStable: false,
        sourceEncoding: 'ASCII',
        zoomCoverageChart: false
    }

    stage('Extract Memcheck') {
        sh 'make memcheck'
        archiveArtifacts artifacts: 'valgrind.xml'
        publishValgrind pattern: 'valgrind.xml',
        publishResultsForFailedBuilds: true
    }

    stage('Clean container') {
        sh 'make clean-container'
    }
}
