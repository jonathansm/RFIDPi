
let rfidpi = angular.module('RFIDCardData', []);

rfidpi.factory('tagDatabase', ['$http', '$q', function ($http, $q) {

    let tags = [];
    let allTags = [];
    let selectedTag = {};
    
    $('#shutdownButton').on('click', function (e) {
	    $('#optionsModal').modal('hide');
		$('#shutdownModal').modal('show');
		$http.get('http://' + location.hostname + ':5000/api/shutdown')
			.then(function (res) {
        });
})

$('#restartButton').on('click', function (e) {
	$('#optionsModal').modal('hide');
	$('#restartModal').modal('show');
    $http.get('http://' + location.hostname + ':5000/api/restart')
        .then(function (res) {
        });
})

    $http.get('http://' + location.hostname + ':5000/api/tags')
        .then(function (res) {
            for (let tag in res.data) 
            {
                tags.push(res.data[tag]);
            }
        });

    let getSelectedTag = function () {
        return selectedTag;
    };

    let selectTag = function (tag) {
        angular.copy(tag, selectedTag);
    };

    let getAll = function () {
        return tags;
    };

    let destoryDatabase = function (){
        console.log("destory_database");
    }

    let updateDatabase = function () {
        let newTags = [];
        $http.get('http://' + location.hostname + ':5000/api/tags')
        .then(function (res) {
            for (let tag in res.data) 
            {
                newTags.push(res.data[tag]);
            }
            console.log("newTags " + newTags);
        
        });
        return newTags;
    };

    return {
        getSelected: getSelectedTag,
        selectTag: selectTag,
        getAll: getAll,
        destoryDatabase: destoryDatabase,
        updateDatabase: updateDatabase
    };
}]);



rfidpi.controller('DataTableController', ['$scope', '$http', '$interval', 'tagDatabase', function ($scope, $http, $interval, tagDatabase) {

    $scope.sorts = {
        bits: 'bits',
        binary_value: 'binary_value',
        hex_value: 'hex_value',
        facility_code: 'facility_code',
        unique_code: 'unique_code',
        proxmark: 'proxmark',
        date_scanned: 'scanned'
    };

    $scope.rightArrow = 'glyphicon-menu-right';

    $scope.tags = tagDatabase.getAll();
    $scope.selectedTag = tagDatabase.getSelected();
    $scope.currView = 'table';
    $scope.sortVal = 'scanned';
    $scope.reverse = true;
    $scope.propertyName = '';

    $interval(checkForUpdate, 5000);

    $scope.sortBy = function(propertyName) {
        $scope.reverse = ($scope.propertyName === propertyName) ? !$scope.reverse : false;
        $scope.propertyName = propertyName;
        $scope.sortVal = $scope.sorts[propertyName];
    };

    $scope.selectTag = function (tag) {
        console.log("Selected Tag " + tag);
        tagDatabase.selectTag(tag);
    };

    deletingTag = 0;

    $scope.deleteTag = function (tag) {
        deletingTag = 1;
        console.log("Deleted Tag " + tag);
        $http.delete('http://' + location.hostname + ':5000/api/tag/' + tag.id)
        .then(function (res) {
            console.log("Tag Deleted");
            $scope.tags = tagDatabase.updateDatabase();
            deletingTag = 0;
        });
    };

    $scope.destoryDatabase = function () {
        tagDatabase.destoryDatabase();
    };

    lastestID = 0;
    function checkForUpdate() {
        if (deletingTag == 0)
        {
            $http.get('http://' + location.hostname + ':5000/api/tag/latest')
            .then(function (res) {
                newID = res.data[0]['id'];
                if (newID > lastestID && lastestID != 0)
                {
                    console.log("New data, table needs to be updated");
                    $scope.tags = tagDatabase.updateDatabase();
                }
                lastestID = newID;
            });
        }
    }
}]);
