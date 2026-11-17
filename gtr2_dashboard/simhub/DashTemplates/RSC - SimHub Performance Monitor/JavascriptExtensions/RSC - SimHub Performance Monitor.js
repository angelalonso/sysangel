function toggleErrorButton() {

	let togglecondition = $prop('InputStatus.GraphicalDashPlugin.ErrorReadoutToggle');

    if (root.togglei == null) {
		root.togglei = false;
        root.togglej = true;
    }

    if (togglecondition && root.togglej) {
        root.togglei = !root.togglei;
        root.togglej = false;
    }

    if (!togglecondition) {root.togglej = true; }

    return root.togglei ? 100 : 0;
}

function errorGlow() {
	let errorCount = $prop('DataCorePlugin.LoggingErrorsCount');
	if (root['newError'] == null){ root['newError'] = errorCount;}
	if ($prop('InputStatus.GraphicalDashPlugin.ErrorReadoutToggle')) { root['newError'] = errorCount;}
	return root['newError'] != errorCount;
}

function errorString(mode)
{
	let errorCount = $prop('DataCorePlugin.LoggingErrorsCount');
	if (root['errorCountOld'] == null){ root['errorCountOld'] = errorCount;}
	if (root['outputString'] == null){ root['outputString'] = '';}
	if (root['outputStringFull'] == null){ root['outputStringFull'] = '';}

	if (errorCount > root['errorCountOld']) {
		var separator = '--- Error ' + (errorCount) + ' ---\n';
		root['outputString'] = separator + $prop('DataCorePlugin.LoggingLastMessage');

		var separator2 = '\n \n' + '--- Error ' + (errorCount) + ' ---\n';
		root['outputStringFull'] += separator2 + $prop('DataCorePlugin.LoggingLastMessage');
	}

	root['errorCountOld'] = errorCount;

	return mode == "full" ? (root['outputStringFull'] == '' ? 'NO ERROR \n' : root['outputStringFull']) : root['outputString'];
}